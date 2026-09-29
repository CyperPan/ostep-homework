CC     := gcc
CFLAGS := -Wall -O2

PROGS := syscall ctxswitch

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS)
