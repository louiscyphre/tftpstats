//
//      TFTPS main file
//      ~~~~~~~~~~~~~~~~
//
//      filename:   tftps_main.c
//      project:    tftps
//
//      authors:    AKS, GMG
//                  Technion, Spring 2009
//

// Included Headers ////////////////////////////////////////////////////////////
#include "tftps_config.h"

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
// using malloc(), free(), exit(), EXIT_FAILURE
#include <string.h>
// using memset(), strncpy()
#include <unistd.h>
// using close()
#include <sys/stat.h>
#include <fcntl.h>
// using open(), O_WRONLY
#include <stdbool.h>
// using true
#include <errno.h>
// using errno, EINTR

#include "tftp_types.h"
#include "tftp_lib.h"
#include "tftp_resources.h"
#include "tftps_main.h"


// Implementation //////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
    char *args[] = {"tftps", "6969"};  // default command line params
    int result = 0, data_packet_size = 0, data_packet_number = 0,
            packet_size = 0, server_failures = 0, saved_errno = 0;
#ifndef DEBUG
    char ip_str[INET_ADDRSTRLEN];       // a buffer for an IP address string
#endif
    rrq_packet_t rrq_packet;
//    ack_packet_t ack_packet;
    dat_packet_t dat_packet;
    resources_t resources;  // all the resources that need treatment on exit
    bool failure = false;   // is used to exit inner loops to the main loop


    result = allocate_resources(&resources);
    CHECK_PRINT_EXIT(result,
                     "Server initialization failed: not enough memory.\n");

    prepare_program_arguments(args, argc, argv);
    printf("Starting with arguments: %s\n", args[1]);


    result = prepare_socket_and_address(&resources.server_socket,
                                        &resources.server_address,
                                        NULL, args[1]);
    CHECK_PRINT_EXIT(result, "Server initialization failed.\n");

    // the next function call performs all the preparations needed for the
    // sender_address to be used in subsequent recvfrom() call
    result = prepare_sender_address(&resources.sender_address);
    CHECK_PRINT_EXIT(result,
                     "Server initialization failed: not enough memory.\n");

    printf("Server initialization successful.\n");

    server_failures = 0;
    do  // the outer infinite loop, jumps back to the "wait for a request" stage
    {
        failure = false;    // is used in inner loops to exit to this loop

        printf("Listening on port: %d\n", get_port(&resources.server_address));

        // receive an RRQ and return SUCCESS or return FAILURE if couldn't
        result = recieve_packet(&resources, OPCODE_RRQ, &packet_size,
                                MAX_RECVFROM_RETRIES,
                                MAX_INVALID_PACKETS, MIN_PACKET_SIZE);
        if (result != TFTP_SUCCESS) {
            break;  // receive_packet() couldn't receive an RRQ, terminate
        }           // (breaking the main loop terminates the server)

        decode_rrq(&rrq_packet, resources.buffer);
#ifdef DEBUG
        printf("IN:RRQ, %s, %s\n", rrq_packet.file_name, rrq_packet.mode_name);
#else
        printf("An RRQ packet has been received successfully from "
               "%s : %d (%d bytes).\n"
               "filename: %s, mode: %s\n",
               get_ip_str(&resources.sender_address, ip_str),
               get_port(&resources.sender_address),
               packet_size, rrq_packet.file_name, rrq_packet.mode_name);
#endif

        resources.fd = open(rrq_packet.file_name, O_RDONLY | O_NOCTTY);
        saved_errno = errno;    // save errno for later analysis
        if (resources.fd == -1) {
#ifdef DEBUG
            perror("TFTP_ERROR");
            flush_socket(resources.server_socket);
#else
            perror("Error: open");
            flush_socket_verbose(resources.server_socket);
#endif
            // allow the next file access errors
            switch (saved_errno) {
                case EACCES:    // fall through all the cases
                case EISDIR:
                case ELOOP:
                case ENAMETOOLONG:
                case ENFILE:
                case ENOENT:
                    continue;   // continue to listen on the port
                    break;
            }

            // treat other errors as server failures
            if (++server_failures < MAX_SERVER_FAILURES) {
                continue;       // continue to listen on the port
            }

            break;              // break to terminate the server
        }

        usec_stopper();     // the first call is needed for initialization

        data_packet_number = 0;
        do  // data transmission loop
        {
            // prepare the next file chunk for transmission
            result = read(resources.fd, resources.buffer, MAX_DATA_SIZE);
            if (result == -1) {
                failure = true;
                server_failures++;
#ifdef DEBUG
                perror("TFTP_ERROR");
                flush_socket(resources.server_socket);
#else
                perror("Error: read");
                flush_socket_verbose(resources.server_socket);
#endif
                break;  // break will make server continue listening on the port
            }
#ifdef DEBUG
            printf("READING: %d\n", result);
#endif

            data_packet_size = result + DAT_HEADER_SIZE;
            // allow block numbers start over at 1 if MAX_BLOCK_NUMBER reached
            data_packet_number %= MAX_BLOCK_NUMBER;
            data_packet_number++;       // update the progress counter
/// suggested error check & server_failures++:
            encode_dat(&dat_packet, data_packet_number,
                       resources.buffer, result);

            // the next call returns if the DATa was sent and ACKnowled or error
            result = send_data_packet(&dat_packet, &resources, PACKET_TIMEOUT,
                                      MAX_TIMEOUTS, MAX_SENDTO_RETRIES);
            switch (result) {
                case TFTP_ERROR_RECVFROM:          // fall through
                case TFTP_ERROR_SELECT:
                    failure = true; // set flag that will lead out to main loop
                    server_failures++;
                    close(resources.fd);
                    break;
                case TFTP_MAX_TIMEOUTS_REACHED:    // fall through
                case TFTP_MAX_INVALID_PACKETS_REACHED:
                    failure = true; // set flag that will lead out to main loop
//                    transmission_failures++;
                    close(resources.fd);
                    break;
                case TFTP_SUCCESS:
                    failure = false;
#ifndef DEBUG
                    printf("File transfer rate: %.2f KB/sec\n", // KB not KiB !
                           (double) data_packet_size / usec_stopper() * 1000);
#endif
                    break;
                default:                            // just in case
                    failure = true; // set flag that will lead out to main loop
                    server_failures++;
                    close(resources.fd);
                    break;
            }

            // the transmission loop breaks after the last chunk was sent or failure
        } while (data_packet_size == DAT_HEADER_SIZE + MAX_DATA_SIZE &&
                 !failure);

        if (!failure) {
#ifdef DEBUG
            printf("SENDOK\n");
#else
            printf("The file has been sent successfully.\n");
#endif
            close(resources.fd);
            server_failures = 0;    // on success reset the failure counter
        }
#ifdef DEBUG
        else
        {
            fprintf(stderr, "SENDFAIL\n");
        }
#endif

    } while (server_failures < MAX_SERVER_FAILURES);

    free_resources(&resources);
    printf("The amount of server failures has reached the allowed maximum.\n"
           "All the resources have been released. Terminating.\n");
    fflush(stdout);

    return TFTP_SUCCESS;
}


// Service Functions ///////////////////////////////////////////////////////////

void prepare_program_arguments(char *args[], int argc, char *argv[]) {
    int i;

    if (argc < 1 || argc > 2) {
        fprintf(stderr, "Usage: %s [port]\n"
                        "\tport - the port number to listen on\n"
                        "\t       the default port is 69.", args[0]);
        exit(EXIT_FAILURE);
    }

    // if argc > 1 redirect the args[] pointers to the argv[] elements
    // i must be under 2 because it's the number of args[] elements
    for (i = 1; i < argc && i < 2; i++) {
        args[i] = argv[i];
    }
}


//
// returns:     TFTP_ERROR_RECVFROM
//              TFTP_MAX_INVALID_PACKETS_REACHED
//              TFTP_SUCCESS

int recieve_packet(resources_t *resources_ptr, const uint16_t requested_opcode,
                   int *packet_size_ptr, const int max_recv_retries,
                   const int max_invalid_packets, const int min_packet_size) {
    int result = 0, recvfrom_retries = 0, invalid_packets = 0;
    int16_t packet_type = 0;

    do {
        result = recvfrom(resources_ptr->server_socket, resources_ptr->buffer,
                          resources_ptr->buffer_size, 0,
                          resources_ptr->sender_address.ai_addr,
                          &resources_ptr->sender_address.ai_addrlen);
        if (result == -1) {
            // check if interrupted by a signal and whether a retry is allowed
            if (errno == EINTR && ++recvfrom_retries < max_recv_retries) {
#ifdef DEBUG
                fprintf(stderr,
                        "TFTP_ERROR: Interrupted by a signal. Retrying.\n");
                // the data could be read partially - flush the socket
                flush_socket(resources_ptr->server_socket);
#else
                fprintf(stderr, "Interrupted by a signal. Retrying.\n");
                // the data could be read partially - flush the socket
                flush_socket_verbose(resources_ptr->server_socket);
#endif
                continue;   // continue listening on the port or retry receiving
            }
#ifdef DEBUG
            perror("TFTP_ERROR");
#else
            perror("Error: recvfrom");
#endif
            return TFTP_ERROR_RECVFROM;
        }

        *packet_size_ptr = result;  // return the packet size to caller

        recvfrom_retries = 0;       // on success - reset failures count

        if (result < min_packet_size) {
#ifdef DEBUG
            fprintf(stderr, "FLOWERROR: a packet was received, "
                   "but it's too short (%d bytes).\n", result);
            flush_socket(resources_ptr->server_socket);
#else
            fprintf(stderr, "Error: a packet was received, but it's too short "
                            "(%dB).\n", result);
            flush_socket_verbose(resources_ptr->server_socket);
#endif
            if (++invalid_packets < max_invalid_packets) {
                continue;   // continue listening on the port
            }
            return TFTP_MAX_INVALID_PACKETS_REACHED;
        }

        // if we are here - the packet is big enough to have an opcode,
        // try to decode it
        packet_type = get_packet_type(resources_ptr->buffer);
        if (packet_type != requested_opcode) {
#ifdef DEBUG
            fprintf(stderr,
                    "FLOWERROR: an unexpected packet has been received.\n");
            flush_socket(resources_ptr->server_socket);
#else
            fprintf(stderr, "Error: an unexpected packet has been received "
                            "(size: %dB, type: %d)\n", result, packet_type);
            flush_socket_verbose(resources_ptr->server_socket);
#endif
            if (++invalid_packets < max_invalid_packets) {
                continue;   // continue listening on the port
            }
            return TFTP_MAX_INVALID_PACKETS_REACHED;
        }
    } while (false); // if we get here - a valid packet was received

    return TFTP_SUCCESS;
}


//
//  returns:    TFTP_ERROR_RECVFROM
//              TFTP_ERROR_SELECT
//              TFTP_MAX_TIMEOUTS_REACHED
//              TFTP_MAX_INVALID_PACKETS_REACHED
//              TFTP_FAILURE
//
int send_data_packet(dat_packet_t *dat_packet_ptr, resources_t *resources_ptr,
                     const int packet_timeout, const int max_timeouts,
                     const int max_send_retries) {
    int result = 0, timeout_counter = 0, packet_size = 0, sendto_retries = 0;
#ifndef DEBUG
    char ip_str[INET_ADDRSTRLEN];       // a buffer for an IP address string
#endif


    // send the data packet until the matching ACK packet arrives or
    // until maximum timeouts is reached
    do {
        // send to SENDER ADDRESS, that's where the request packet came from
        result = sendto(resources_ptr->server_socket, (void *) (dat_packet_ptr),
                        DAT_HEADER_SIZE + dat_packet_ptr->data_size, 0,
                        resources_ptr->sender_address.ai_addr,
                        resources_ptr->sender_address.ai_addrlen);

        if (result == -1) {
            // check if interrupted by a signal and whether a retry is allowed
            if (errno == EINTR && ++sendto_retries < max_send_retries) {
#ifndef DEBUG
                printf("Interrupted by a signal. Retrying.\n");
#endif
                continue;   // continue listening on the port or retry receiving
            }
#ifdef DEBUG
            perror("TFTP_ERROR");
#else
            perror("Error: sendto");
#endif
            return TFTP_ERROR_RECVFROM;
        }

        sendto_retries = 0; // on success - reset failures count

#ifdef DEBUG
        printf("OUT:DATA, %d, %d\n", get_packet_number(dat_packet_ptr), result);
#else
        printf("Data packet no.%d sent to: %s : %d (%d bytes)\n",
               get_packet_number(dat_packet_ptr),
               get_ip_str(&resources_ptr->sender_address, ip_str),
               get_port(&resources_ptr->sender_address), result);
#endif
        switch (wait_for_packets(resources_ptr->server_socket,
                                 packet_timeout)) {
            case TFTP_ERROR_SELECT:
#ifdef DEBUG
                perror("TFTP_ERROR");
#else
                perror("Error: select");
#endif
                return TFTP_ERROR_SELECT;
            case TFTP_TIMEOUT:
                if (++timeout_counter == max_timeouts) {
#ifdef DEBUG
                    fprintf(stderr, "FLOWERROR: the maximal number of "
                           "failures has been reached.\n");
#else
                    fprintf(stderr, "Error: the maximal number of timeouts "
                                    "has been reached.\n");
#endif
                    return TFTP_MAX_TIMEOUTS_REACHED;
                }
#ifndef DEBUG
                printf("Resending data packet no.%d\n",
                       get_packet_number(dat_packet_ptr));
#endif
                continue;       // send DATa packet again
                break;
            case TFTP_SUCCESS:
                break;
        }

#ifndef DEBUG
        // there was a response
        timeout_counter = 0;
#endif

        // get the packet that is waiting on the socket, no error tolerance here
        result = recieve_packet(resources_ptr, OPCODE_ACK, &packet_size,
                                0,                  //MAX_RECVFROM_RETRIES
                                0,                  //MAX_INVALID_PACKETS,
                                ACK_PACKET_SIZE);   //MIN_PACKET_SIZE

        // decode the packet and check if it's the ACK that we expect to receive
        if (result == TFTP_SUCCESS &&
            get_packet_type(resources_ptr->buffer) == OPCODE_ACK) {
#ifdef DEBUG
            printf("IN:ACK, %d\n",get_packet_number(resources_ptr->buffer));
#else
            printf("ACK  packet no.%d received from %s : %d.\n",
                   get_packet_number(resources_ptr->buffer),
                   get_ip_str(&resources_ptr->sender_address, ip_str),
                   get_port(&resources_ptr->sender_address));
#endif
            if (get_packet_number(resources_ptr->buffer) ==
                get_packet_number(dat_packet_ptr)) {
                return TFTP_SUCCESS;
            } else    // the packet was ACK with wrong block number in it
            {
#ifdef DEBUG
                fprintf(stderr, "FLOWERROR: the ACK packet block number is %d, "
                        "instead of %d.\n",
                        get_packet_number(resources_ptr->buffer),
                        get_packet_number(dat_packet_ptr));
                flush_socket(resources_ptr->server_socket);
#else
                fprintf(stderr, "Error: the ACK packet block number is %d, "
                                "instead of %d.\n",
                        get_packet_number(resources_ptr->buffer),
                        get_packet_number(dat_packet_ptr));
                flush_socket_verbose(resources_ptr->server_socket);
#endif
                // in the next line we continue using timeout_counter as a total
                // failure counter according to the exercise guidelines,
                // otherwise an invalid_packets counter could be used
                timeout_counter++;
            }
        }

        // no invalid packets allowed
        if (result == TFTP_MAX_INVALID_PACKETS_REACHED) {
            return result;
        }
    } while (timeout_counter < max_timeouts);

    return TFTP_FAILURE;   // we get here on failure only
}
