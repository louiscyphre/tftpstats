//
// Filename: main
// Created by: louiscyphre@github
// Created on: 20:12 03.07.2018
//
#include "tftpstats_config.h"

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

typedef enum {
    TFTPSTATS_FAILURE = -1, TFTPSTATS_SUCCESS, TFTPSTATS_INVALID_PARAMETER,
    TFTPSTATS_ERROR_GETADDRINFO, TFTPSTATS_COULD_NOT_CONNECT,
    TFTPSTATS_ERROR_SENDTO, TFTPSTATS_UNEXPECTED_PACKET,
    TFTPSTATS_ERROR_RECVFROM, TFTPSTATS_PACKET_SENT_PARTIALLY,
    TFTPSTATS_ERROR_SELECT, TFTPSTATS_TIMEOUT,
    TFTPSTATS_ERROR_OPEN, TFTPSTATS_MAX_TIMEOUTS_REACHED,
    TFTPSTATS_MAX_INVALID_PACKETS_REACHED,
    TFTPSTATS_FILE_NAME_TOO_LONG, TFTPSTATS_MODE_NAME_TOO_LONG,
    TFTPSTATS_OUT_OF_MEMORY
} tftpstats_result_t;


void
callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
}

int main() {
    
    char error_buffer[PCAP_ERRBUF_SIZE];
    char *device;

    device = pcap_lookupdev(error_buffer);
    if (device == NULL) {
        printf("%s\n", error_buffer);
        exit(1);
    }

    pcap_t *pcap_resource;
    pcap_resource = pcap_open_live(device, BUFSIZ, 0, -1, error_buffer);
    if (pcap_resource == NULL) {
        printf("pcap_open_live(): %s\n", error_buffer);
        exit(1);
    }
    pcap_loop(pcap_resource, -1, callback, NULL);
    return 0;
}
