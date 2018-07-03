//
//      TTFTP functions library, interface
//      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//      filename:   ttftp_lib.h
//      project:    ttftp library
//
//      authors:    AKS, GMG
//                  Technion, Spring 2009
//
#ifndef __TTFTP_LIB_H
#define __TTFTP_LIB_H

// Switches ////////////////////////////////////////////////////////////////////

//#define DEBUG        // if defined, the program will match the HW standard,
// otherwise there will be more detailed output and
// more error-tolerant server behaviour

#define FLUSH_REPORTS   // if defined flush_socket() will output detailed info
// about its status and actions


// Included Headers ////////////////////////////////////////////////////////////

#include <netdb.h>
// using struct addrinfo
#include <stdint.h>
// using uint16_t


// Macros //////////////////////////////////////////////////////////////////////

#define CHECK_PRINT_EXIT(result, message)           \
{                                                   \
    if((result) != TTFTP_SUCCESS)                   \
    {                                               \
        fprintf(stderr, message);                   \
        fflush(stderr);                             \
        exit(result);                               \
    }                                               \
}


// Declarations ////////////////////////////////////////////////////////////////

// the next function allocates memory for a struct addrinfo, address_ptr will
// point to the new memory block upon return, use free_address() to release it
//
// if address_str is null the socket will be created for ANY_ADDR (0.0.0.0)
// and only in this case it will be bound to the given port_str
// if address_str is not NULL the socket will not be bound to any port
int prepare_socket_and_address(int *socket_ptr, struct addrinfo *address_ptr,
                               const char *address_str, const char *port_str);

// the next function allocates memory, use free_address() to release it
int prepare_sender_address(struct addrinfo *sender_address_ptr);

void free_address(struct addrinfo *address_ptr);

int encode_rrq(rrq_packet_t *rrq_ptr, const char *file_name,
               const char *mode_name);

int decode_rrq(rrq_packet_t *rrq_ptr, const void *packet_ptr);

int encode_dat(dat_packet_t *dat_packet_ptr, uint16_t block_number,
               const void *data, int data_size);

int decode_dat(dat_packet_t *dat_ptr, const void *packet_ptr,
               const int packet_size);

int encode_ack(ack_packet_t *ack_ptr, uint16_t block_number);

int decode_ack(ack_packet_t *ack_ptr, const void *packet_ptr);

int get_packet_type(void *packet_ptr);

int get_packet_number(void *packet_ptr);

int get_packet_size(void *packet_ptr);

int wait_for_packets(int socket, const long timeout);

int flush_socket(int socket);

int flush_socket_verbose(int socket);  // reports success and failure

// the next function uses a buffer (ip_str) of INET_ADDRSTRLEN size, that must
// be preallocated by user
// the function changes its contents and returns a pointer to its first byte
const char *get_ip_str(struct addrinfo *addr_ptr, char *ip_str);

unsigned int get_port(struct addrinfo *addr_ptr);

// the next function uses static variables to keep the time of the last run
// it was originally designed to calculate data transfer rates
// on its first run it will return the number of uSec since the current Epoch
// i.e. since 00:00:00 UTC 1 Jan 1970, but on the subsequent runs it will return
// the time in uSec that passed since its previous run
unsigned long usec_stopper(void);


#endif // __TTFTP_LIB_H
