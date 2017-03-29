
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mach.h>

struct integer_message {
    mach_msg_header_t head;
    mach_msg_type_t type;
    int32_t inline_integer;
};

int32_t receive_integer(mach_port_t port) {
    mach_msg_return_t err;
    struct integer_message msg;

    msg.head.msgh_size = sizeof(struct integer_message);
    err = mach_msg( &(msg.head), MACH_RCV_MSG,
            0, msg.head.msgh_size, port,
            MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if(err != MACH_MSG_SUCCESS) {
        exit(err);
    }
    return msg.inline_integer;
}

void send_integer(mach_port_t port, int32_t integer) {
    mach_msg_return_t err;
    struct integer_message msg;

    msg.head.msgh_bits = MACH_MSGH_BITS_REMOTE(
            MACH_MSG_TYPE_MAKE_SEND);
    msg.head.msgh_size = sizeof(struct integer_message);
    msg.head.msgh_local_port = MACH_PORT_NULL;
    msg.head.msgh_remote_port = port;

    msg.type.msgt_name = MACH_MSG_TYPE_INTEGER_32;
    msg.type.msgt_size = 32;
    msg.type.msgt_number = 1;
    msg.type.msgt_inline = TRUE;
    msg.type.msgt_longform = FALSE;
    msg.type.msgt_deallocate = FALSE;

    msg.inline_integer = integer;

    err = mach_msg( &(msg.head), MACH_SEND_MSG,
            msg.head.msgh_size, 0, MACH_PORT_NULL,
            MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if(err != MACH_MSG_SUCCESS) {
        exit(err);
    }
}

int main(int argc, char *argv[]) {
    if(argc && argv) { }
    mach_port_t port;
    kern_return_t ret;
    int s = 42;
    int r;

    ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
    if(ret != KERN_SUCCESS) {
        perror("mach_port_allocate");
        exit(1);
    }

    send_integer(port, s);
    r = receive_integer(port);
    printf("%d\n", r);

    return 0;
}

