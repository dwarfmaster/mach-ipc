
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

static struct bpf_insn bpf_filter[] =
{
    {NETF_IN|NETF_BPF, 0, 0, 0},		/* Header. */
    {BPF_LD|BPF_H|BPF_ABS, 0, 0, 12},		/* Load Ethernet type */
    {BPF_JMP|BPF_JEQ|BPF_K, 2, 0, 0x0806},	/* Accept ARP */
    {BPF_JMP|BPF_JEQ|BPF_K, 1, 0, 0x0800},	/* Accept IPv4 */
    {BPF_JMP|BPF_JEQ|BPF_K, 0, 1, 0x86DD},	/* Accept IPv6 */
    {BPF_RET|BPF_K, 0, 0, 1500},		/* And return 1500 bytes */
    {BPF_RET|BPF_K, 0, 0, 0},			/* Or discard it all */
};
static int bpf_filter_len = sizeof (bpf_filter) / sizeof (short);

int main(int argc, char *argv[]) {
    file_t f;
    mach_port_t sel;
    kern_return_t ret;
    error_t err;
    char buf[4096];
    typeinfo_t tp;
    mach_msg_type_number_t amount;
    device_t device;
    size_t count;
    char* data;
    mach_port_t rcv;

    f = file_name_lookup("/dev/eth0", O_READ, 0);
    if(f == MACH_PORT_NULL) {
        perror("file_name_lookup");
        goto exit;
    }

    ret = device_open(f, D_READ | D_WRITE, "eth", &device);
    if(ret != KERN_SUCCESS) {
        perror("device_open");
        goto exit;
    }

    /* Getting MAC address */
    int address[2];
    count = 2;
    ret = device_get_status(device, NET_ADDRESS, address, &count);
    if(ret != KERN_SUCCESS) {
        perror("device_get_status");
        goto exit;
    }
    printf("sizeof(int) : %d\n", sizeof(int));
    printf("Count : %d\n", count);
    printf("Address : %08X%08X\n",
            address[0], address[1]);

    /* Reading using filter */
    ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &rcv);
    if(ret != KERN_SUCCESS) {
        perror("mach_port_allocate");
        goto exit;
    }
    ret = device_set_filter(device, rcv, MACH_MSG_TYPE_MAKE_SEND, 0,
            (unsigned short*)bpf_filter, bpf_filter_len);
    if(ret != KERN_SUCCESS) {
        perror("device_set_filter");
        goto exit;
    }

    for(;;) {
        if(!receive_data(rcv, &tp, buf, 4096)) continue;
        printf("Received : %d %d (%d)\n", tp.id, tp.size, tp.number);

        int cmd;
        for(cmd = 0; cmd < 64; ++cmd) {
            if(cmd % 4 == 0) printf("\n");
            printf("%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX ",
                    buf[cmd * 8],     buf[cmd * 8 + 1], buf[cmd * 8 + 2], buf[cmd * 8 + 3],
                    buf[cmd * 8 + 4], buf[cmd * 8 + 5], buf[cmd * 8 + 6], buf[cmd * 8 + 7]);
        }
        printf("\n");
        fflush(stdout);
    }

exit:
    device_close(device);
    mach_port_deallocate(mach_task_self(), rcv);
    mach_port_deallocate(mach_task_self(), f);

    return 0;
}

