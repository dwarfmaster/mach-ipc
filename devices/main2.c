
#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <hurd.h>
#include <hurd/io.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>

#define PRINT_INS_NUMBER(ins) printf("%s : %d\n", #ins, ins)

int main(int argc, char *argv[]) {
    PRINT_INS_NUMBER(NETF_PUSHLIT);
    PRINT_INS_NUMBER(NETF_PUSHZERO);
    PRINT_INS_NUMBER(NETF_PUSHWORD);
    PRINT_INS_NUMBER(NETF_PUSHHDR);
    PRINT_INS_NUMBER(NETF_PUSHIND);
    PRINT_INS_NUMBER(NETF_PUSHHDRIND);
    PRINT_INS_NUMBER(NETF_PUSHSTK);
    PRINT_INS_NUMBER(NETF_NOPUSH);
    return 0;
}

