//
//      TFTP resources data type, interface
//      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//      filename:   tftp_resources.h
//      project:    tftp library
//
//      authors:    AKS, GMG
//                  Technion, Spring 2009
//
#ifndef __TFTP_RESOURCES_H
#define __TFTP_RESOURCES_H

// Included Headers ////////////////////////////////////////////////////////////

#include <netdb.h>
// using struct addrinfo


// Types ///////////////////////////////////////////////////////////////////////

struct resources {
    struct addrinfo client_address;     // stores client address info
    struct addrinfo server_address;     // stores server address info
    struct addrinfo sender_address;     // stores the last packet sender info
    int client_socket;      // stores client open socket descriptor
    int server_socket;      // stores server open socket descriptor
    int fd;                 // stores currently open file descriptor
    void *buffer;             // points to the packet reception buffer
    int buffer_size;        // holds the packet recept. buffer size
};

typedef struct resources resources_t;


// Declarations ////////////////////////////////////////////////////////////////

int allocate_resources(resources_t *resources_ptr);

void free_resources(resources_t *resources_ptr);


#endif // __TFTP_RESOURCES_H
