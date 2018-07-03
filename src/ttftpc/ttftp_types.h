//
//      TTFTP types definitions
//      ~~~~~~~~~~~~~~~~~~~~~~~
//
//      filename:   ttftp_types.c
//      project:    ttftp library
//
//      authors:    AKS, GMG
//                  Technion, Spring 2009
//
#ifndef __TTFTP_TYPES_H
#define __TTFTP_TYPES_H

// Included Headers ////////////////////////////////////////////////////////////

#include <stdint.h>
// using uint8_t, uint16_t

// Types ///////////////////////////////////////////////////////////////////////

typedef enum {
    TTFTP_FAILURE = -1, TTFTP_SUCCESS, TTFTP_INVALID_PARAMETER,
    TTFTP_ERROR_GETADDRINFO, TTFTP_COULD_NOT_CONNECT,
    TTFTP_ERROR_SENDTO, TTFTP_UNEXPECTED_PACKET,
    TTFTP_ERROR_RECVFROM, TTFTP_PACKET_SENT_PARTIALLY,
    TTFTP_ERROR_SELECT, TTFTP_TIMEOUT,
    TTFTP_ERROR_OPEN, TTFTP_MAX_TIMEOUTS_REACHED,
    TTFTP_MAX_INVALID_PACKETS_REACHED,
    TTFTP_FILE_NAME_TOO_LONG, TTFTP_MODE_NAME_TOO_LONG,
    TTFTP_OUT_OF_MEMORY
} ttftp_result_t;

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

#endif // __TTFTP_TYPES_H
