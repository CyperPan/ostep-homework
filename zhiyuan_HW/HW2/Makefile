CC     := gcc
CFLAGS := -Wall -g

PROGS := q1 q2 q3 q4 q5 q6 q7 q8 syscall ctxswitch

all: $(PROGS)

# chapter 6 measurement programs are built with optimization
syscall ctxswitch: CFLAGS := -Wall -O2

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS) q2.output a.out
