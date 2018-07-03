//
//      TFTPS main header file
//      ~~~~~~~~~~~~~~~~~~~~~~~
//
//      filename:   tftps_main.h
//      project:    tftps
//
//      authors:    AKS, GMG
//                  Technion, Spring 2009
//
#ifndef __TFTPS_MAIN_H
#define __TFTPS_MAIN_H

// Declarations ////////////////////////////////////////////////////////////////

void prepare_program_arguments(char *args[], int argc, char *argv[]);

int recieve_packet(resources_t *resources_ptr, const uint16_t requested_opcode,
                   int *packet_size_ptr, const int max_recv_retries,
                   const int max_invalid_packets, const int min_packet_size);

int recieve_request(resources_t *resources_ptr, const int max_retries,
                    const int min_packet_size);

int send_data_packet(dat_packet_t *dat_packet_ptr, resources_t *resources_ptr,
                     const int packet_timeout, const int max_timeouts,
                     const int max_send_retries);


#endif // __TFTPS_MAIN_H
