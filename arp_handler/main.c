
#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <hurd.h>
#include <hurd/io.h>
#include <mach.h>
#include <device/device.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include "proto.h"
#include "ports.h"

void make_request(mach_port_t port, char* ip) {
    arp_query_t* query;
    char buffer[256];
    typeinfo_t tpinfo;

    query = (arp_query_t*)buffer;
    query->type = 0x0800;
    memcpy(query->addr, ip, 4);
    tpinfo.id   = arp_query;
    tpinfo.size = 6;
    send_data(port, &tpinfo, buffer);
}

int main(int argc, char* argv[]) {
    file_t file;
    typeinfo_t tpinfo;
    mach_port_t arp, tmp;
    kern_return_t ret;
    char buffer[2048];
    arp_register_t* reg;
    char myip[4] = {0x00, 0x00, 0x00, 0x00};
    char ips[][4] = {
        {129, 199, 129, 30}, /* TROENE  */
        {129, 199, 129, 22}, /* TAGETTE */
        {129, 199, 129, 33}, /* TAMARIN */
    };

    if(argc < 1) exit(1);

    file = file_name_lookup(argv[1], O_READ | O_WRITE, 0);
    if(file == MACH_PORT_NULL) {
        printf("file_name_lookup\n");
        return 1;
    }

    ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &arp);
    if(ret != KERN_SUCCESS) {
        printf("mach_port_allocate\n");
        return 1;
    }

    reg = (arp_register_t*)buffer;
    reg->port_type.msgt_name          = MACH_MSG_TYPE_MAKE_SEND;
    reg->port_type.msgt_size          = sizeof(mach_port_t) * 8;
    reg->port_type.msgt_number        = 1;
    reg->port_type.msgt_inline        = TRUE;
    reg->port_type.msgt_longform      = FALSE;
    reg->port_type.msgt_deallocate    = FALSE;
    reg->port_type.msgt_unused        = 0;
    reg->port                         = arp;

    reg->content_type.msgt_name       = MACH_MSG_TYPE_UNSTRUCTURED;
    reg->content_type.msgt_size       = 8;
    reg->content_type.msgt_number     = 8;
    reg->content_type.msgt_inline     = TRUE;
    reg->content_type.msgt_longform   = FALSE;
    reg->content_type.msgt_deallocate = FALSE;
    reg->content_type.msgt_unused     = 0;
    reg->type                         = 0x0800;
    reg->len                          = 4;
    memcpy(reg->data, myip, 4);

    send_data_low(file, sizeof(arp_register_t) + 8, buffer, arp_register);

    make_request(file, ips[0]);
    make_request(file, ips[1]);
    make_request(file, ips[2]);

    while(1) {
        tmp = arp;
        if(!receive_data(&tmp, &tpinfo, buffer, 2048)) continue;
        if(tpinfo.id != arp_answer) {
            printf("Invalid ARP answer received\n");
            continue;
        }

        printf("%hhu.%hhu.%hhu.%hhu -> %02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\n",
                buffer[0], buffer[1], buffer[2], buffer[3], 
                buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9]); 

    }
}

