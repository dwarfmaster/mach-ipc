
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cthreads.h>
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
        fprintf(stderr, "Couldn't receive\n");
        exit(err);
    }
    fflush(stdout);
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
        fprintf(stderr, "Couldn't send %d\n", integer);
        exit(err);
    }
    fflush(stdout);
}


/* Only works on x86 32-bit */
void thread_fork(void*(*routine)(void*), void* arg) {
    vm_size_t stack_size = vm_page_size * 16;
    kern_return_t ret;
    thread_t thread;

    printf("%d %d\n", vm_page_size, stack_size);
    ret = thread_create(mach_task_self(), &thread);
    if(ret != KERN_SUCCESS) {
        perror("thread_create");
        exit(1);
    }

    /* Setting the state of the thread */
    struct i386_thread_state state;
    unsigned int count;

    count = i386_THREAD_STATE_COUNT;
    ret = thread_get_state(thread, i386_REGS_SEGS_STATE,
            (thread_state_t) &state, &count);
    if(ret != KERN_SUCCESS) {
        perror("thread_get_state");
        exit(1);
    }

    /* Create stack */
    vm_address_t stack;
    ret = vm_allocate(mach_task_self(), &stack, stack_size, TRUE);
    if(ret != KERN_SUCCESS) {
        perror("vm_allocate");
        exit(1);
    }
    /* Protect the lowest page against access to detect stack overflow */
    ret = vm_protect(mach_task_self(),
            stack,                /* The address */
            vm_page_size,         /* The size */
            FALSE,                /* Set the current permissions, not the maximum
                                     permissions allowed */
            VM_PROT_NONE);        /* Give no permission */
    if(ret != KERN_SUCCESS) {
        perror("vm_protect");
        exit(1);
    }
    /* Setup the stack */
    int* top   = (int*)(stack + stack_size) - 2;
    *top       = 0;        /* The return address */
    *(top + 1) = (int)arg; /* The argument */

    state.eip  = (int) routine; /* Start on routine */
    state.uesp = (int) top;     /* Set stack pointer */
    state.ebp  = 0;             /* Clear frame pointer*/

    ret = thread_set_state(thread, i386_REGS_SEGS_STATE,
            (thread_state_t) &state, i386_THREAD_STATE_COUNT);
    if(ret != KERN_SUCCESS) {
        perror("thread_set_state");
        exit(1);
    }
    ret = thread_resume(thread);
    if(ret != KERN_SUCCESS) {
        perror("thread_resume");
        exit(1);
    }
}

int another_function() {
    int x = 5;
    int y = 6;
    x = y;
    return x;
}

void* routine(void* buf) {
    mach_port_t port = *(mach_port_t*)buf;
    sleep(3);
    send_integer(port, another_function());

    /* Thread must terminate itself
     * It can't return, because the return address
     * is set to 0.
     */
    thread_terminate(mach_thread_self());
}

int main(int argc, char *argv[]) {
    if(argc && argv) { }
    mach_port_t port;
    kern_return_t ret;

    ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
    if(ret != KERN_SUCCESS) {
        perror("mach_port_allocate");
        exit(1);
    }

    thread_fork(routine, &port);
    int r = receive_integer(port);
    printf("Received %d\n", r);

    mach_port_deallocate(mach_task_self(), port);
    return 0;
}

