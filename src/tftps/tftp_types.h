//
//      TFTP types definitions
//      ~~~~~~~~~~~~~~~~~~~~~~~
//
//      filename:   tftp_types.h
//      project:    tftp library
//
//      authors:    AKS, GMG
//                  Technion, Spring 2009
//
#ifndef __TFTP_TYPES_H
#define __TFTP_TYPES_H

// Included Headers ////////////////////////////////////////////////////////////

#include <stdint.h>
// using uint8_t, uint16_t

// Types ///////////////////////////////////////////////////////////////////////

typedef enum {
    TFTP_FAILURE = -1, TFTP_SUCCESS, TFTP_INVALID_PARAMETER,
    TFTP_ERROR_GETADDRINFO, TFTP_COULD_NOT_CONNECT,
    TFTP_ERROR_SENDTO, TFTP_UNEXPECTED_PACKET,
    TFTP_ERROR_RECVFROM, TFTP_PACKET_SENT_PARTIALLY,
    TFTP_ERROR_SELECT, TFTP_TIMEOUT,
    TFTP_ERROR_OPEN, TFTP_MAX_TIMEOUTS_REACHED,
    TFTP_MAX_INVALID_PACKETS_REACHED,
    TFTP_FILE_NAME_TOO_LONG, TFTP_MODE_NAME_TOO_LONG,
    TFTP_OUT_OF_MEMORY
} tftp_result_t;

struct rrq_packet {
    uint16_t opcode;    // stored in the network endianness
    uint8_t text[FILE_NAME_MAX_LEN + MODE_NAME_MAX_LEN + 2]; // + 2 for 2 '\0's
    // the next two fields are for local use only, stored in the host endianness
    char *file_name; // points to the beginning of a file name in text[]
    char *mode_name; // points to the beginning of a mode name in text[]
} __attribute__((packed));

typedef struct rrq_packet rrq_packet_t;


struct wrq_packet {
    uint16_t opcode;    // stored in the network endianness
    uint8_t text[FILE_NAME_MAX_LEN + MODE_NAME_MAX_LEN + 2]; // + 2 for 2 '\0's
    // the next two fields are for local use only, stored in the host endianness
    char *file_name; // points to the beginning of a file name in text[]
    char *mode_name; // points to the beginning of a mode name in text[]
} __attribute__((packed));

typedef struct wrq_packet wrq_packet_t;


struct ack_packet {
    uint16_t opcode;    // stored in the network endianness
    uint16_t number;    // stored in the network endianness
} __attribute__((packed));

typedef struct ack_packet ack_packet_t;


struct dat_packet {
    uint16_t opcode;    // stored in the network endianness
    uint16_t number;    // stored in the network endianness
    uint8_t data[MAX_DATA_SIZE];
    uint16_t data_size; // stored in the host endianness, for local use only
} __attribute__((packed));

typedef struct dat_packet dat_packet_t;

#endif // __TFTP_TYPES_H
