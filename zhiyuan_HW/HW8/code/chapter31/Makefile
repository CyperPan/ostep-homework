# The chapter 31 programs. On macOS, common_threads.h supplies the semaphore
# (see the comment there); on Linux it uses the system's sem_t.
CC = gcc
CFLAGS = -Wall -O2 -pthread
ALL = fork-join rendezvous barrier reader-writer reader-writer-nostarve mutex-nostarve \
      fork-join-broken barrier-broken reader-writer-nostarve-weak

all: $(ALL)

%: %.c common_threads.h rw_test.h
	$(CC) $(CFLAGS) -o $@ $<

fork-join-broken: fork-join.c common_threads.h
	$(CC) $(CFLAGS) -DBROKEN -o $@ $<

barrier-broken: barrier.c common_threads.h
	$(CC) $(CFLAGS) -DBROKEN -o $@ $<

reader-writer-nostarve-weak: reader-writer-nostarve.c common_threads.h rw_test.h
	$(CC) $(CFLAGS) -DWEAK_TURNSTILE -o $@ $<

clean:
	rm -f $(ALL)
