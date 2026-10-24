CC     := gcc
CFLAGS := -Wall -O2

PROGS := tlb timer tlb-O0 tlb-naive

all: $(PROGS)

tlb: tlb.c
	$(CC) $(CFLAGS) -o $@ $<

timer: timer.c
	$(CC) $(CFLAGS) -o $@ $<

# Q5: the same program without optimization ...
tlb-O0: tlb.c
	$(CC) -Wall -O0 -o $@ $<

# ... and a naive version (read-only loop, result unused) that -O2 deletes
tlb-naive: tlb.c
	$(CC) $(CFLAGS) -Wno-unused-but-set-variable -DNAIVE -o $@ $<

clean:
	rm -f $(PROGS)
