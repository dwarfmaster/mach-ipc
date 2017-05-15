
CFLAGS=-Wall -Wextra -I../../ENS_sysres/lib
LDFLAGS=
CC=gcc
PROG=ipc-test.out
OBJS=main.o ../../ENS_sysres/lib/lib.a

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@touch $(PROG) $(OBJS)
	rm -f $(PROG) $(OBJS)

rec: clean all

.PHONY: all clean rec

