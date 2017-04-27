
#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <hurd.h>
#include <hurd/io.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>

typedef struct typeinfo {
    unsigned int id;
    unsigned int size;
    unsigned int number; /* Number of elements of that type */
} typeinfo_t;

struct message_full_header {
    mach_msg_header_t head;
    mach_msg_type_t type;
};

int send_data(mach_port_t port, const typeinfo_t* info, char* data) {
    mach_msg_return_t err;
    struct message_full_header* hd = (struct message_full_header*)data;
    unsigned int size = info->size * info->number;
    memmove(data + sizeof(struct message_full_header), data, size);

    hd->head.msgh_bits = MACH_MSGH_BITS_REMOTE(
            MACH_MSG_TYPE_MAKE_SEND);
    hd->head.msgh_size = size + sizeof(struct message_full_header);
    hd->head.msgh_local_port = MACH_PORT_NULL;
    hd->head.msgh_remote_port = port;

    hd->type.msgt_name = info->id;
    hd->type.msgt_size = info->size;
    hd->type.msgt_number = info->number;
    hd->type.msgt_inline = TRUE;
    hd->type.msgt_longform = FALSE;
    hd->type.msgt_deallocate = FALSE;

    err = mach_msg( &hd->head, MACH_SEND_MSG,
            hd->head.msgh_size, 0, MACH_PORT_NULL,
            MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if(err != MACH_MSG_SUCCESS) return 0;
    return 1;
}

int receive_data(mach_port_t port, typeinfo_t* info, char* buffer, size_t size) {
    mach_msg_return_t err;
    struct message_full_header* hd = (struct message_full_header*)buffer;
    hd->head.msgh_size = size;
    err = mach_msg( &hd->head, MACH_RCV_MSG,
            0, hd->head.msgh_size, port,
            MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if(err != MACH_MSG_SUCCESS) return 0;

    info->id     = hd->type.msgt_name;
    info->size   = hd->type.msgt_size;
    info->number = hd->type.msgt_number;
    memmove(buffer, buffer + sizeof(struct message_full_header), info->size * info->number);
    return 1;
}

int main(int argc, char *argv[]) {
    file_t f;
    mach_port_t sel;
    kern_return_t ret;
    error_t err;
    uint32_t buf[4096];
    typeinfo_t tp;
    mach_msg_type_number_t amount;

    if(argc != 2) return 1;

    f = file_name_lookup(argv[1], O_READ, 0);
    if(f == MACH_PORT_NULL) {
        perror("file_name_lookup");
        exit(1);
    }

    ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &sel);
    if(ret != KERN_SUCCESS) {
        perror("mach_port_allocate");
        exit(1);
    }

    __io_select_request(f, sel, SELECT_READ);
    receive_data(sel, &tp, buf, 4090);
    printf("%u %u %u\n", tp.id, tp.size, tp.number);
    printf("%d\n", buf[0]);

    io_readable(f, &amount);
    printf("%d\n", amount);
    if(amount == 0) goto exit;

    io_read(f, &buf, &amount, -1, amount);
    buf[amount] = '\0';
    printf("%s\n", buf);

exit:
    mach_port_deallocate(mach_task_self(), f);

    return 0;
}

