//
// Filename: main
// Created by: louiscyphre@github
// Created on: 20:12 03.07.2018
//
#include "tftpstats_config.h"
#include "debug_utils.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pcap/pcap.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>

// Types ///////////////////////////////////////////////////////////////////////
typedef enum { SUCCESS, FAILURE, INVALID_PARAMETER, TIMEOUT,
    OUT_OF_MEMORY } return_value_t;

void
callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
}

int main(int argc,char **argv) {

    char error_buffer[PCAP_ERRBUF_SIZE];
    pcap_if_t *all_devices = NULL;
    int result;

    result = pcap_findalldevs(&all_devices, error_buffer);
    if (result == -1) {
        LOG();
        printf("[ERROR] %s\n", error_buffer);
        exit(FAILURE);
    }

    bpf_u_int32 ip_address, subnet_mask;
    const char *device = all_devices->name;

    pcap_lookupnet(device, &ip_address, &subnet_mask, error_buffer);

    pcap_t *pcap_resource = pcap_open_live(device, CAPTURE_BUFFER_SIZE,
                                           0, -1, error_buffer);

    if (pcap_resource == NULL) {
        LOG();
        printf("[ERROR] pcap_open_live(): %s\n", error_buffer);
        exit(FAILURE);
    }

    struct bpf_program filter_program;
    result = pcap_compile(pcap_resource, &filter_program,
                          argv[1], 0, ip_address);
    if (result == -1) {
        LOG();
        fprintf(stderr, "[ERROR] Call of pcap_compile() failed\n");
        exit(FAILURE);
    }

    if (pcap_setfilter(pcap_resource, &filter_program) == -1) {
        LOG();
        fprintf(stderr, "[ERROR] Call of pcap_setfilter() failed\n");
        exit(FAILURE);
    }

    pcap_loop(pcap_resource, -1, callback, NULL);

    pcap_freealldevs(all_devices);

    return SUCCESS;
}
