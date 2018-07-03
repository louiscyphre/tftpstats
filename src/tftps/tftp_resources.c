//
//      TFTP resources data type, implementation
//      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//      filename:   tftp_resources.c
//      project:    tftp library
//
//      authors:    AKS, GMG
//                  Technion, Spring 2009
//

// Included Headers ////////////////////////////////////////////////////////////
#include "tftps_config.h"

#include <stdlib.h>
// using malloc(), free()
#include <string.h>
// using memset()
#include <unistd.h>
// using close()
#include <errno.h>
// using errno, EBADF


#include "tftp_types.h"
// using TFTP error codes
#include "tftp_lib.h"
// using free_address()
#include "tftp_resources.h"


// Declarations ////////////////////////////////////////////////////////////////

// Note: usually, there's no reason to call initialize_resources() explicitly,
//       it's being called in the beginning of the allocate_resources() and at
//       the end of the free_resources() function
static int initialize_resources(resources_t *resources_ptr);


// Implementation //////////////////////////////////////////////////////////////

static int initialize_resources(resources_t *resources_ptr) {
    if (resources_ptr == NULL) {
        return TFTP_INVALID_PARAMETER;
    }

    // zero all the pointer fields, all the other fields get zeroed for company
    memset(resources_ptr, 0, sizeof(resources_t));

    // assign illegal values for free_resources() to distinguish used
    // descriptors from unused ones
    resources_ptr->server_socket = -1;
    resources_ptr->fd = -1;

    // all the other fields of resources_t structure are supposed to be
    // initialized further in the code, at this moment they are just zeroed
    return TFTP_SUCCESS;
}


int allocate_resources(resources_t *resources_ptr) {
    if (resources_ptr == NULL) {
        return TFTP_INVALID_PARAMETER;
    }

    initialize_resources(resources_ptr);

    // allocate a generic buffer for a packet reception,
    // twice the data packet size should suffice any needs
    resources_ptr->buffer_size = 2 * (DAT_HEADER_SIZE + MAX_DATA_SIZE);
    resources_ptr->buffer = malloc(resources_ptr->buffer_size);
    if (resources_ptr->buffer == NULL) {
        return TFTP_OUT_OF_MEMORY;
    }

    return TFTP_SUCCESS;
}


void free_resources(resources_t *resources_ptr) {
    const int max_close_retries = 7;
    int close_retries = 0;


    if (resources_ptr != NULL) {
        free(resources_ptr->buffer);
        free_address(&resources_ptr->server_address);
        free_address(&resources_ptr->sender_address);
        // socket and file descriptors are initialized by allocate_resources()
        // to be -1 so that free_resources() could know which file descriptors
        // were used by the program and which were not
        if (resources_ptr->server_socket != -1)      // if the socket was opened
        {
            while (close(resources_ptr->server_socket) == -1 && errno != EBADF
                   && ++close_retries < max_close_retries);
        }

        if (resources_ptr->server_socket != -1)      // if the file was opened
        {
            while (close(resources_ptr->fd) == -1 && errno != EBADF
                   && ++close_retries < max_close_retries);
        }

        // reinitialize the structure against subsequent free_resources() calls
        initialize_resources(resources_ptr);
    }
}
