
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cthreads.h>
#include <mach.h>

/* Highly inefficient malloc */
void* malloc(vm_size_t size) {
    vm_address_t ad;
    kern_return_t ret;
    vm_size_t* new;
    vm_size_t ns = size + sizeof(vm_size_t);

    ret = vm_allocate(mach_task_self(), &ad, ns, TRUE);
    if(ret != KERN_SUCCESS) return NULL;
    new = (vm_size_t*)ad;
    *new = ns;
    ++new;
    return (void*)new;
}

void free(void* addr) {
    vm_size_t* ptr = addr;
    --ptr;
    vm_deallocate(mach_task_self(), (vm_address_t)ptr, *ptr);
}

struct test {
    long long a, lot, of, data;
};

int main(int argc, char *argv[]) {
    if(argc && argv) { }
    struct test* v;
    v = malloc(sizeof(struct test));
    v->a = 156;
    v->lot = 42;
    printf("%lld %lld\n", v->a, v->lot);
    free(v);

    return 0;
}

