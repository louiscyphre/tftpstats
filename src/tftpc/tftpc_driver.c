//
//      TFTPC main file
//      ~~~~~~~~~~~~~~~~
//
//      filename:   tftpc_main.c
//      project:    tftpc
//
//      authors:    AKS, GMG
//                  Technion, Spring 2009
//

// Included Headers ////////////////////////////////////////////////////////////

#include "tftpc_config.h"

#include <arpa/inet.h>
// using htons(), ntohs(), inet_ntop()
#include <sys/types.h>
#include <sys/socket.h>
// using recvfrom(), sendto()
#include <netdb.h>
// using getaddrinfo(), struct addrinfo

#include <stdio.h>
// using printf(), fprintf(), perror(), stderr
#include <stdlib.h>
// using exit(), EXIT_FAILURE
#include <string.h>
// using strncpy()
#include <unistd.h>
// using close()
#include <sys/stat.h>
#include <fcntl.h>
// using open(), O_WRONLY
#include <stdbool.h>
// using true

#include "tftp_types.h"
#include "tftp_lib.h"
#include "tftp_resources.h"

// Declarations ////////////////////////////////////////////////////////////////

void prepare_program_arguments(char *args[], int argc, char *argv[]);


// Implementation //////////////////////////////////////////////////////////////

int tftpc_run(int argc, char *argv[]) {
    resources_t resources;              // the application resources structure
    int result = 0,
            packet_size = 0,
            packet_type = 0,
            packet_number = 0,
            client_failures = 0,
            timeout_counter = 0,
            last_data_packet_size = 0,
            last_data_packet_number = 0;
    char *args[] = {"tftpc", "127.0.0.1", "16770", "file.tst"}; // test defaults
    char ip_str[INET_ADDRSTRLEN];       // a buffer for an IP address string
    bool no_reply = true;
    rrq_packet_t rrq_packet;
    ack_packet_t ack_packet;
    dat_packet_t dat_packet;


    result = allocate_resources(&resources);
    CHECK_PRINT_EXIT(result,
                     "Client initialization failed: not enough memory.\n");

    prepare_program_arguments(args, argc, argv);
    printf("Starting with arguments: %s : %s %s\n", args[1], args[2], args[3]);

    // the next function call performs all the preparations needed for the
    // server_address to be used in subsequent sendto() call
    result = prepare_socket_and_address(&resources.client_socket,
                                        &resources.server_address,
                                        args[1], args[2]);
    CHECK_PRINT_EXIT(result, "Client initialization failed.\n");

    // the next function call performs all the preparations needed for the
    // sender_address to be used in subsequent recvfrom() call
    result = prepare_sender_address(&resources.sender_address);
    CHECK_PRINT_EXIT(result, "Client initialization failed.\n");

    // prepare the packet for dispatch
    encode_rrq(&rrq_packet, args[3], "octet");
    packet_size = get_packet_size(&rrq_packet);

    timeout_counter = 0;
    client_failures = 0;
    no_reply = true;
    do  // send RRQ
    {
        printf("Sending data request...\n");
        result = sendto(resources.client_socket, (void *) (&rrq_packet),
                        packet_size, 0,
                        resources.server_address.ai_addr,
                        resources.server_address.ai_addrlen);

        if (result == -1) {
            perror("Error: sendto");
            free_resources(&resources);
            fflush(stderr);
            return TFTP_ERROR_SENDTO;
        }

        if (result != packet_size) {
            fprintf(stderr, "Error: %d bytes out of %d were sent.\n",
                    result, packet_size);
            free_resources(&resources);
            fflush(stderr);
            return TFTP_PACKET_SENT_PARTIALLY;
        }

        printf("Data request sent successfully (%d bytes).\n"
               "Waiting for a data packet...\n", result);

        // if there's no reply in PACKET_TIMEOUT seconds - repeat data request
        switch (wait_for_packets(resources.client_socket, PACKET_TIMEOUT)) {
            case TFTP_ERROR_SELECT:
                perror("Error: select");
                free_resources(&resources);
                fflush(stderr);
                return TFTP_ERROR_SELECT;
                break;
            case TFTP_TIMEOUT:
                if (++timeout_counter == MAX_TIMEOUTS) {
                    fprintf(stderr, "Error: the maximal number of timeouts"
                                    " has been reached.\n");
                    free_resources(&resources);
                    fflush(stderr);
                    return TFTP_MAX_TIMEOUTS_REACHED;
                }
                break;
            case TFTP_SUCCESS:
                no_reply = false;
                break;
            default:
                fprintf(stderr,
                        "Error: unexpected failure during RRQ sending.\n");
                free_resources(&resources);
                fflush(stderr);
                return TFTP_FAILURE;
        }

        if (++client_failures == MAX_TIMEOUTS + 1) // anti-infinite-loop feature
        {
            // normally we shouldn't get here
            fprintf(stderr, "Error: unexpected failure during RRQ sending.\n");
            free_resources(&resources);
            fflush(stderr);
            return TFTP_FAILURE;
        }
    } while (no_reply);

    // open for write, create if not exists, truncate if exists, set 0600 mode
    resources.fd = open(args[3], O_WRONLY | O_CREAT | O_TRUNC,
                        S_IRUSR | S_IWUSR);
    if (resources.fd == -1) {
        perror("Error: open");
        free_resources(&resources);
        fflush(stderr);
        return TFTP_ERROR_OPEN;
    }

    usec_stopper();              // the first run is needed for initialization

    last_data_packet_number = 0; // the first block is number 1, 0 precedes it
    last_data_packet_size = 0;
    do  // receive the file
    {
        packet_size = recvfrom(resources.client_socket, resources.buffer,
                               resources.buffer_size, 0,
                               resources.sender_address.ai_addr,
                               &resources.sender_address.ai_addrlen);

        if (packet_size == -1) {
            perror("Error: recvfrom");
            free_resources(&resources);
            fflush(stderr);
            return TFTP_ERROR_RECVFROM;
        }

        if (packet_size < 2) {
            fprintf(stderr, "Error: the packet received is too short "
                            "(%d bytes).\n", packet_size);
            free_resources(&resources);
            fflush(stderr);
            return TFTP_UNEXPECTED_PACKET;
        }

        // the packet is big enough to have an opcode, let's try to decode it
        packet_type = get_packet_type(resources.buffer);
        if (packet_type != OPCODE_DAT || packet_size < DAT_HEADER_SIZE ||
            packet_size > DAT_HEADER_SIZE + MAX_DATA_SIZE) {
            fprintf(stderr, "Error: an unexpected packet received (size: %d "
                            "bytes, type: %d).\n", packet_size, packet_type);
            free_resources(&resources);
            fflush(stderr);
            return TFTP_UNEXPECTED_PACKET;
        }

        decode_dat(&dat_packet, resources.buffer, packet_size);
        packet_number = get_packet_number(&dat_packet);
        printf("Data packet no.%d received from %s : %d (%d bytes).\n",
               packet_number, get_ip_str(&resources.sender_address, ip_str),
               get_port(&resources.sender_address), packet_size);

        // write to file and advance only if the next data packet is received
        if (packet_number == last_data_packet_number + 1 ||
            // allow block numbers start over at 1 if MAX_BLOCK_NUMBER reached
            (packet_number == 1 &&
             last_data_packet_number == MAX_BLOCK_NUMBER)) {
            // update the progress tracking variables
            last_data_packet_size = packet_size;
            last_data_packet_number = packet_number;
            write(resources.fd, dat_packet.data, dat_packet.data_size);

            printf("Download rate: %.2f KB/sec\n",
                   (double) last_data_packet_size / usec_stopper() * 1000);
        } else {
            fprintf(stderr, "Error: The data packet block number is %d, "
                            "expected %d.\n", packet_number,
                    last_data_packet_number + 1);
            flush_socket_verbose(resources.client_socket);
        }

/// NOTE: if the first "data packet" is invalid then the ACK no.0 will be sent
///       to the server, because that's what the last_data_packet_number will be
///       if this is not the behavoiur intended, for example you think it's
///       better to send an RRQ again - replace this NOTE by a handling code

        // data packet has been received successfully, send ACKs until the next
        // packet arrives or until maximum timeouts is reached
        encode_ack(&ack_packet, last_data_packet_number);
        packet_size = get_packet_size(&ack_packet);

        timeout_counter = 0;
        no_reply = true;
        do {
            result = sendto(resources.client_socket, (void *) (&ack_packet),
                            packet_size, 0,
                            resources.server_address.ai_addr,
                            resources.server_address.ai_addrlen);
            printf("ACK  packet no.%d sent to: %s : %d\n",
                   ntohs(ack_packet.number),
                   get_ip_str(&resources.server_address, ip_str),
                   get_port(&resources.server_address));

            // no need to wait_for_packets if the last data packet was received
            if (last_data_packet_size < DAT_HEADER_SIZE + MAX_DATA_SIZE) {
                break;
            }

            switch (wait_for_packets(resources.client_socket, PACKET_TIMEOUT)) {
                case TFTP_ERROR_SELECT:
                    perror("Error: select");
                    free_resources(&resources);
                    fflush(stderr);
                    return TFTP_ERROR_SELECT;
                    break;
                case TFTP_TIMEOUT:
                    printf("Resending ACK for data packet no.%d\n",
                           ntohs(dat_packet.number));

                    if (++timeout_counter == MAX_TIMEOUTS) {
                        fprintf(stderr, "Error: the maximal number of timeouts"
                                        " has been reached.\n");
                        free_resources(&resources);
                        fflush(stderr);
                        return TFTP_MAX_TIMEOUTS_REACHED;
                    }
                    break;
                case TFTP_SUCCESS:
                    no_reply = false;
                    break;
            }
        } while (no_reply);
    } while (last_data_packet_size == DAT_HEADER_SIZE + MAX_DATA_SIZE);

    free_resources(&resources);
    printf("Operation complete. All sockets closed.\n");
    fflush(stdout);

    return TFTP_SUCCESS;
}


// Service Functions ///////////////////////////////////////////////////////////

void prepare_program_arguments(char *args[], int argc, char *argv[]) {
    int i;

    if (argc < 1 || argc > 4) {
        fprintf(stderr, "Usage: %s host port file\n"
                        "\thost - a TFTP server name or its IP address\n"
                        "\tport - a TFTP server port number\n"
                        "\tfile - a name of a file on the TFTP server\n",
                args[0]);
        exit(EXIT_FAILURE);
    }

    // if argc > 1 redirect the args[] pointers to the argv[] elements
    // i must be under 4 because it's the number of args[] elements
    for (i = 1; i < argc && i < 4; i++) {
        args[i] = argv[i];
    }
}
