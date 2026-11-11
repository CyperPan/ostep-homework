CC     := gcc
CFLAGS := -Wall -O2 -pthread

PROGS := timer counter list hash

all: $(PROGS)

%: %.c common.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS)
