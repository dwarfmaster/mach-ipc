
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cthreads.h>
#include <mach.h>

/* Prevent problems in case a and b have side effects
 * when evaluated
 */
#define max(a,b) \
    ({ __typeof__(a) _a = (a); \
       __typeof__(b) _b = (b); \
       _a > _b ? _a : _b; })

integer_t micros(time_value_t v) {
    return v.seconds * 1000000 + v.microseconds;
}

int main(int argc, char *argv[]) {
    if(argc && argv) { }
    task_t child;
    task_t self = mach_task_self(); /* Receive a send right to the port
                                     * representing the task
                                     */
    kern_return_t ret;

    /* Creating a new task
     * The new task is empty, ie no threads.
     * A new thread should be created in it for it to do
     * something
     */
    ret = task_create(self, TRUE, &child);
    if(ret != KERN_SUCCESS) {
        perror("task_create");
        exit(1);
    }

    /* Terminating a task */
    ret = task_terminate(child);
    if(ret != KERN_SUCCESS) {
        /* Can only fail if child is not a task */
        perror("task_terminate");
        exit(1);
    }

    /* Display info about self */
    size_t size = 1 + max(max(sizeof(struct task_basic_info),
                              sizeof(struct task_thread_times_info)),
                              sizeof(struct task_events_info));
    task_info_t info = malloc(size);
    if(info == NULL) {
        perror("malloc");
        exit(1);
    }
    mach_msg_type_number_t count = size / sizeof(natural_t);

    struct task_basic_info* basic_info
        = (struct task_basic_info*)info;
    struct task_thread_times_info* times_info
        = (struct task_thread_times_info*)info;
    struct task_events_info* events_info
        = (struct task_events_info*)info;

    ret = task_info(self, TASK_BASIC_INFO, info, &count);
    if(ret == KERN_SUCCESS) {
        printf("Creation time : %d\n", basic_info->creation_time.seconds);
    }
    count = size;
    ret = task_info(self, TASK_THREAD_TIMES_INFO, info, &count);
    if(ret == KERN_SUCCESS) {
        printf("User time: %d\nSystem time : %d\n", micros(times_info->user_time),
                                                    micros(times_info->system_time));
    }
    count = size;
    ret = task_info(self, TASK_EVENTS_INFO, info, &count);
    if(ret == KERN_SUCCESS) {
        printf("Messages sent: %d\n", events_info->messages_sent);
        printf("Messages received: %d\n", events_info->messages_received);
    } else if(ret == KERN_INVALID_ARGUMENT) {
        /* KERN_INVALID_ARGUMENT is returned in case of to small size,
         * contrarily to what doc says.
         */
        fprintf(stderr, "info too small for events\n");
    }

    free(info);

    return 0;
}

