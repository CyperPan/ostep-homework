CC     := gcc
CFLAGS := -Wall -g          # -g: debug symbols for gdb and valgrind

PROGS := null leak overflow use_after_free bad_free vector

all: $(PROGS)

vector: vector.c
	$(CC) $(CFLAGS) -O2 -o $@ $<

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS) vgcore.* core
