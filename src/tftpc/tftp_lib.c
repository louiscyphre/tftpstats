//
//      TFTP functions library, implementation
//      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//      filename:   tftp_lib.c
//      project:    tftp library
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
// using malloc(), free()
#include <string.h>
// using strncpy()
#include <unistd.h>
// using close(), exit(), EXIT_FAILURE
#include <sys/select.h>
// using select(), FD_ZERO(), FD_SET()
#include <errno.h>
// using errno, EINTR
#include <stropts.h>
// using ioctl(), I_NREAD
#include <stdbool.h>
// using true
#include <sys/time.h>
// using gettimeofday()
#include <sys/ioctl.h>
// using FIONREAD

#include "tftp_types.h"
#include "tftp_lib.h"


// Implementation //////////////////////////////////////////////////////////////

// the next function allocates memory, use free_address() to release it
int prepare_socket_and_address(int *socket_ptr, struct addrinfo *address_ptr,
                               const char *address_str, const char *port_str) {
    struct addrinfo address_hints, *address_list, *current;
    int result, socket_fd;
#ifndef DEBUG
    char ip_str[INET_ADDRSTRLEN];
#endif


    if (socket_ptr == NULL || address_ptr == NULL || port_str == NULL) {
        return TFTP_INVALID_PARAMETER;
    }

    // prepare a set of hints or preferences for the subsequent address lookup
    memset(&address_hints, 0, sizeof(struct addrinfo));
    address_hints.ai_family = AF_INET;    // we need TCP/IP v4 addresses
    address_hints.ai_socktype = SOCK_DGRAM; // we need a UDP socket
    address_hints.ai_protocol = 0;          // any protocol is OK
    address_hints.ai_flags = AI_PASSIVE; // wildcard IP address is OK
    // all other fields are zeroed

    // get a list of addresses according to the hints provided
    result = getaddrinfo(address_str, port_str, &address_hints, &address_list);
    if (result != 0) {
#ifdef DEBUG
        fprintf(stderr, "TFTP_ERROR: %s.\n", gai_strerror(result));
#else
        fprintf(stderr, "Error: getaddrinfo: %s.\n", gai_strerror(result));
#endif
        fflush(stderr);
        return TFTP_ERROR_GETADDRINFO;
    }

    // go through the list and connect to the first valid address
    for (current = address_list; current != NULL; current = current->ai_next) {
#ifndef DEBUG
        if (address_str == NULL) {
            printf("Creating a client socket for: %s\n",
                   get_ip_str(current, ip_str));
        } else {
            printf("Creating a server socket for: %s\n",
                   get_ip_str(current, ip_str));
        }
        fflush(stdout);
#endif
        socket_fd = socket(current->ai_family, current->ai_socktype,
                           current->ai_protocol);
        if (socket_fd == -1) {
#ifdef DEBUG
            perror("TFTP_ERROR");
#else
            perror("Error: socket");
#endif
            fflush(stderr);
            continue;
        }

        // the next line checks if the function was called for a socket that is
        // intended for binding, by checking if a NULL address_str was provided,
        // which means that the socket is for listening on a local port
        if (address_str == NULL &&
            bind(socket_fd, current->ai_addr, current->ai_addrlen) == -1) {
            close(socket_fd);
#ifdef DEBUG
            perror("TFTP_ERROR");
#else
            perror("Error: bind");
#endif
            fflush(stderr);
            continue;
        }

        break;
    }

    // check if the loop above has ended because all the addresses were checked
    // and no suitable address was found
    if (current == NULL) {
#ifdef DEBUG
        fprintf(stderr, "TFTP_ERROR: Could not connect.\n");
#else
        fprintf(stderr, "Error: could not connect.\n");
#endif
        fflush(stderr);
        freeaddrinfo(address_list);// free the memory allocated by getaddrinfo()
        return TFTP_COULD_NOT_CONNECT;
    }

    // return (copy) the address structure that was picked during the search
    *address_ptr = *current;
    // take care of the pointer fields: first NULL initialize them...
    address_ptr->ai_addr = NULL;
    address_ptr->ai_canonname = NULL;
    address_ptr->ai_next = NULL;

    // ...then check if there's any data that these fields should point to...
    if (current->ai_addr !=
        NULL) {                                                          // ALLOCATION (!)
        // ...if there is - make a copy of whatever there is
        address_ptr->ai_addr = malloc(sizeof(struct sockaddr));
        if (address_ptr->ai_addr == NULL) {
            // free the memory allocated by getaddrinfo()
            freeaddrinfo(address_list);
            return TFTP_OUT_OF_MEMORY;
        }
        *address_ptr->ai_addr = *current->ai_addr;  // structure is being copied
    }

    if (current->ai_canonname !=
        NULL) {                                                          // ALLOCATION (!)
        address_ptr->ai_canonname = malloc(strlen(current->ai_canonname) + 1);
        if (address_ptr->ai_canonname == NULL) {
            free(address_ptr->ai_addr); // free the memory allocated above
            // free the memory allocated by getaddrinfo()
            freeaddrinfo(address_list);
            return TFTP_OUT_OF_MEMORY;
        }
        strcpy(address_ptr->ai_canonname, current->ai_canonname);
    }

    // return the socket descriptor that was created during the address search
    *socket_ptr = socket_fd;

    // free the memory allocated by getaddrinfo()
    freeaddrinfo(address_list);

    return TFTP_SUCCESS;
}


int prepare_sender_address(struct addrinfo *sender_address_ptr) {
    struct sockaddr *sender_sockaddr_ptr;

    if (sender_address_ptr == NULL) {
        return TFTP_INVALID_PARAMETER;
    }

    if ((sender_sockaddr_ptr = malloc(sizeof(struct sockaddr))) == NULL) {
        return TFTP_OUT_OF_MEMORY;
    }

    // initialize only the fields that are going to be used
    memset(sender_address_ptr, 0, sizeof(struct addrinfo));
    sender_address_ptr->ai_family = AF_INET;
    sender_address_ptr->ai_socktype = SOCK_DGRAM;
    sender_address_ptr->ai_addrlen = sizeof(struct sockaddr);
    sender_address_ptr->ai_addr = sender_sockaddr_ptr;
    // all other fields are zeroed

    return TFTP_SUCCESS;
}


void free_address(struct addrinfo *address_ptr) {
    if (address_ptr != NULL) {
        free(address_ptr->ai_addr);
        free(address_ptr->ai_canonname);
    }
}


int encode_rrq(rrq_packet_t *rrq_ptr, const char *file_name,
               const char *mode_name) {
    if (rrq_ptr == NULL || file_name == NULL || mode_name == NULL) {
        return TFTP_INVALID_PARAMETER;
    }

    if (strlen(file_name) > FILE_NAME_MAX_LEN) {
        return TFTP_FILE_NAME_TOO_LONG;
    }

    if (strlen(mode_name) > MODE_NAME_MAX_LEN) {
        return TFTP_MODE_NAME_TOO_LONG;
    }

    // initializing structure fields
    rrq_ptr->opcode = htons(OPCODE_RRQ);
    rrq_ptr->file_name = (char *) rrq_ptr->text;
    rrq_ptr->mode_name = (char *) rrq_ptr->text + strlen(file_name) + 1;
    strcpy(rrq_ptr->file_name, file_name);
    strcpy(rrq_ptr->mode_name, mode_name);

    return TFTP_SUCCESS;
}


int decode_rrq(rrq_packet_t *rrq_ptr, const void *packet_ptr) {
    char *file_name, *mode_name;


    if (rrq_ptr == NULL || packet_ptr == NULL) {
        return TFTP_INVALID_PARAMETER;
    }

    file_name = (char *) ((rrq_packet_t *) packet_ptr)->text;
    mode_name =
            (char *) ((rrq_packet_t *) packet_ptr)->text + strlen(file_name) +
            1;

    if (strlen(file_name) > FILE_NAME_MAX_LEN) {
        return TFTP_FILE_NAME_TOO_LONG;
    }

    if (strlen(mode_name) > MODE_NAME_MAX_LEN) {
        return TFTP_MODE_NAME_TOO_LONG;
    }

    rrq_ptr->opcode = htons(OPCODE_RRQ);
    // the next two fields are for local use only, no need to change endianness
    rrq_ptr->file_name = (char *) rrq_ptr->text;
    rrq_ptr->mode_name = (char *) rrq_ptr->text + strlen(file_name) + 1;
    strcpy(rrq_ptr->file_name, file_name);
    strcpy(rrq_ptr->mode_name, mode_name);

    return TFTP_SUCCESS;
}


int encode_dat(dat_packet_t *dat_packet_ptr, uint16_t block_number,
               const void *data, int data_size) {
    if (dat_packet_ptr == NULL || data == NULL ||
        data_size < 0 || data_size > MAX_DATA_SIZE) {
        return TFTP_INVALID_PARAMETER;
    }

    dat_packet_ptr->opcode = htons(OPCODE_DAT);
    dat_packet_ptr->number = htons(block_number);
    memcpy(dat_packet_ptr->data, data, data_size);
    // the next field it's for local use only, no need to change endianness
    dat_packet_ptr->data_size = data_size;

    return TFTP_SUCCESS;
}


int decode_dat(dat_packet_t *dat_ptr, const void *packet_ptr,
               const int packet_size) {
    if (dat_ptr == NULL || packet_ptr == NULL) {
        return TFTP_INVALID_PARAMETER;
    }

    dat_ptr->opcode = ((dat_packet_t *) packet_ptr)->opcode;
    dat_ptr->number = ((dat_packet_t *) packet_ptr)->number;
    memcpy(dat_ptr->data, ((dat_packet_t *) packet_ptr)->data,
           packet_size - DAT_HEADER_SIZE);
    // the next field it's for local use only, no need to change endianness
    dat_ptr->data_size = packet_size - DAT_HEADER_SIZE;

    return TFTP_SUCCESS;
}


int encode_ack(ack_packet_t *ack_ptr, uint16_t block_number) {
    if (ack_ptr == NULL) {
        return TFTP_INVALID_PARAMETER;
    }

    ack_ptr->opcode = htons(OPCODE_ACK);
    ack_ptr->number = htons(block_number);

    return TFTP_SUCCESS;
}


int decode_ack(ack_packet_t *ack_ptr, const void *packet_ptr) {
    if (ack_ptr == NULL || packet_ptr == NULL) {
        return TFTP_INVALID_PARAMETER;
    }

    ack_ptr->opcode = ((ack_packet_t *) packet_ptr)->opcode;
    ack_ptr->number = ((ack_packet_t *) packet_ptr)->number;

    return TFTP_SUCCESS;
}


int get_packet_type(void *packet_ptr) {
    if (packet_ptr == NULL) {
        return -1;
    }
    // the 1st 2 bytes of a packet represent an opcode in the network endianness
    return ntohs(*(uint16_t *) (packet_ptr));
}


int get_packet_number(void *packet_ptr) {
    if (packet_ptr == NULL) {
        return -1;
    }
    // no packet type check is done, it's user's responsibility to make sure the
    // packet is DAT or ACK, i.e. it has a number, before calling this function

    // the second pair of bytes of a packet represent its block number in the
    // network endianness
    return ntohs(*((uint16_t *) packet_ptr + 1));
}


int get_packet_size(void *packet_ptr) {
    switch (get_packet_type(packet_ptr)) {
        case OPCODE_ACK:
            return ACK_PACKET_SIZE;
            break;
        case OPCODE_DAT:
            // the actual packet size can't be calculated in a simple way
            // this function returns the maximal possible size of a data packet:
            // header size (4 bytes) + block size
            return DAT_HEADER_SIZE + MAX_DATA_SIZE;
            break;
        case OPCODE_RRQ:
            // 2 bytes for opcode + file_name len + '\0' + mode_name len + '\0'
            return 2 + strlen(((rrq_packet_t *) packet_ptr)->file_name) + 1
                   + strlen(((rrq_packet_t *) packet_ptr)->mode_name) + 1;
            break;
        case OPCODE_WRQ:
            // 2 bytes for opcode + file_name len + '\0' + mode_name len + '\0'
            return 2 + strlen(((wrq_packet_t *) packet_ptr)->file_name) + 1
                   + strlen(((wrq_packet_t *) packet_ptr)->mode_name) + 1;
            break;
    }

    return TFTP_UNEXPECTED_PACKET;
}


int wait_for_packets(int socket, const long timeout) {
    fd_set socket_set;
    struct timeval packet_timeout = {timeout, 0};
    int result;


    FD_ZERO(&socket_set);
    FD_SET(socket, &socket_set);
    do {
        // select( n_fds, &read_fds, &write_fds, &except_fds, &timeout_struct )
        result = select(socket + 1, &socket_set, NULL, NULL, &packet_timeout);
        if (result == -1) {
            if (errno == EINTR) {
#ifdef DEBUG
                fprintf(stderr,
                        "TFTP_ERROR: Interrupted by a signal. Retrying.\n");
#else
                fprintf(stderr,
                        "Interrupted by a signal at tv_sec: %d\ttv_usec: %d\n",
                        (int) packet_timeout.tv_sec,
                        (int) packet_timeout.tv_usec);
#endif
                continue;
            }
            return TFTP_ERROR_SELECT;
        } else if (result == 0) {
#ifdef DEBUG
            fprintf(stderr, "FLOWERROR: The operation timed out\n");
#else
            fprintf(stderr, "The operation timed out.\n");
#endif
            return TFTP_TIMEOUT;
        } else // if result > 0
        {
#ifndef DEBUG
/// this is more of a debug message, not necessary otherwise
///            printf("A reply has arrived in time.\n");
#endif
            return TFTP_SUCCESS;
        }
    } while (true);
}


int flush_socket(int socket)        // FIONREAD
{
    int buff_size = 1 << 10;   // initial buffer size: 2^10 = 1024
    int result = 0, bytes_pending = 0, bytes_received = 0, safety_counter = 0;
    const int max_iterations = 20;
    void *buff = NULL;

    // allocate buff_size, if unavailable - divide buff_size by 2 and try again
    while ((buff = malloc(buff_size)) == NULL && (buff_size /= 2) > 0);
    if (buff_size == 0) {
        return TFTP_OUT_OF_MEMORY;
    }
#ifdef FLUSH_REPORTS
    printf("[ BEFORE FLUSH: buff_size: %d ]\n", buff_size);
    fflush(stdout);
#endif

    while (safety_counter++ < max_iterations) {
        // check if there's any data on the socket
        result = ioctl(socket, FIONREAD, &bytes_pending);
        if (result == -1) {
            perror("TFTP_ERROR");
            free(buff);
            return TFTP_FAILURE;
        }
#ifdef FLUSH_REPORTS
        printf("[ FLUSHING: result: %d, pending %d bytes ]\n", result,
               bytes_pending);
        fflush(stdout);
#endif

        // read whatever there is pending on the socket
        if (bytes_pending > 0) {
            bytes_received = recv(socket, buff, buff_size, 0);
            if (bytes_received == -1) {
#ifdef DEBUG
                perror("TFTP_ERROR");
#else
                perror("Error: recv");
#endif
                free(buff);
                return TFTP_FAILURE;
            }
#ifdef FLUSH_REPORTS
            printf("[ FLUSHING: bytes received: %d ]\n", bytes_received);
            fflush(stdout);
#endif
        } else {
            break;
        }
    }

#ifdef FLUSH_REPORTS
    result = ioctl(socket, FIONREAD, &bytes_pending);
    printf("[ AFTER FLUSH: result: %d, pending %d bytes ]\n", result,
           bytes_pending);
    fflush(stdout);
#endif
    free(buff);
    return TFTP_SUCCESS;
}


int flush_socket2(int socket)       // I_NREAD
{
    int buff_size = 1 << 10;   // initial buffer size: 2^10 = 1024
    int result = 0, bytes_pending = 0, bytes_received = 0;
    void *buff = NULL;

    // allocate buff_size, if unavailable - divide buff_size by 2 and try again
    while ((buff = malloc(buff_size)) == NULL && (buff_size /= 2) > 0);
    if (buff_size == 0) {
        return TFTP_OUT_OF_MEMORY;
    }

    // while there are messages on the socket
    while ((result = ioctl(socket, I_NREAD, &bytes_pending))) {
        if (result == -1) {
            perror("TFTP_ERROR");
            free(buff);
            return TFTP_FAILURE;
        }
#ifdef FLUSH_REPORTS
        printf("[ BEFORE FLUSH: result: %d, pending %d bytes ]\n", result,
               bytes_pending);
        fflush(stdout);
#endif

        // read whatever there is pending on the socket
        while (bytes_pending >= 0) {
            bytes_received = recv(socket, buff, buff_size, 0);
            if (bytes_received == -1) {
#ifdef DEBUG
                perror("TFTP_ERROR");
#else
                perror("Error: recv");
#endif
                free(buff);
                return TFTP_FAILURE;
            } else if (bytes_received == 0) {
                break;
            }
#ifdef FLUSH_REPORTS
            printf("[ FLUSHING: bytes received %d ]\n", bytes_received);
            fflush(stdout);
#endif
            bytes_pending -= bytes_received;
        }
    }
#ifdef FLUSH_REPORTS
    result = ioctl(socket, FIONREAD, &bytes_pending);
    printf("[ AFTER FLUSH: result: %d, pending %d bytes ]\n", result,
           bytes_pending);
    fflush(stdout);
#endif

    free(buff);
    return TFTP_SUCCESS;
}


int flush_socket3(int socket)       // I_FLUSH
{
    // flush all the read and write queues
    if (ioctl(socket, I_FLUSH, FLUSHRW) == -1) {
#ifdef DEBUG
        perror("TFTP_ERROR");
#else
        perror("Error: ioctl");
#endif
        return TFTP_FAILURE;
    }

    return TFTP_SUCCESS;
}


int flush_socket_verbose(int socket) {
    int result = flush_socket(socket);

    if (result == TFTP_SUCCESS) {
        printf("The socket has been flushed successfully.\n");
    } else {
        printf("The socket flush failed.\n");
    }

    return result;
}


const char *get_ip_str(struct addrinfo *addr_ptr, char *ip_str) {
    if (addr_ptr == NULL || ip_str == NULL) {
        return NULL;
    }

    return inet_ntop(addr_ptr->ai_family,
                     &((struct sockaddr_in *) addr_ptr->ai_addr)->sin_addr,
                     ip_str, INET_ADDRSTRLEN);
}


unsigned int get_port(struct addrinfo *addr_ptr) {
    if (addr_ptr == NULL) {
        return 0;
    }

    return ntohs(((struct sockaddr_in *) addr_ptr->ai_addr)->sin_port);
}


unsigned long usec_stopper(void) {
    static struct timeval last_time = {0, 0};
    static struct timeval current_time = {0, 0};

    last_time = current_time;
    gettimeofday(&current_time, NULL);

    return (current_time.tv_sec - last_time.tv_sec) * 1000000 +
           (current_time.tv_usec - last_time.tv_usec);
}
