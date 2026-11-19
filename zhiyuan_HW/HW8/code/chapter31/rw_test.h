// Test instrumentation for reader-writer.c and reader-writer-nostarve.c
// (added; the lock code itself does not depend on it).
//
// - RW_READ_US / RW_WRITE_US (environment): how long a reader / writer keeps
//   the lock after getting it, in microseconds (default 0). This is the
//   "add sleep() calls" part of the question: the given reader() and writer()
//   below the "don't change" line hold the lock only for one printf.
// - Every acquire checks the rules of a reader-writer lock: a writer is alone,
//   readers only share with readers. A violation stops the program.
// - At exit, the longest wait (call of acquire -> lock held) of any reader and
//   of any writer is printed to stderr, with the number of acquisitions.
#ifndef __rw_test_h__
#define __rw_test_h__

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static atomic_int rw_active_readers, rw_active_writers;
static atomic_long rw_max_wait_ns[2], rw_acquires[2];   // [0] readers, [1] writers
static int rw_hold_us[2];

static long rw_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

static void rw_report(void) {
    fprintf(stderr, "max wait: reader %.1f ms (%ld reads), writer %.1f ms (%ld writes)\n",
            rw_max_wait_ns[0] / 1e6, (long) rw_acquires[0],
            rw_max_wait_ns[1] / 1e6, (long) rw_acquires[1]);
}

static void rw_test_init(void) {
    char *r = getenv("RW_READ_US"), *w = getenv("RW_WRITE_US");
    rw_hold_us[0] = r ? atoi(r) : 0;
    rw_hold_us[1] = w ? atoi(w) : 0;
    atexit(rw_report);
}

// called by acquire, after it got the lock; `start` = rw_now_ns() at the call
static void rw_got_lock(int writer, long start) {
    long wait = rw_now_ns() - start;
    long old = rw_max_wait_ns[writer];
    while (wait > old && !atomic_compare_exchange_weak(&rw_max_wait_ns[writer], &old, wait))
        ;
    rw_acquires[writer]++;
    if (writer) {
        int w = ++rw_active_writers;
        if (w != 1 || rw_active_readers != 0) {
            fprintf(stderr, "VIOLATION: writer with %d writers, %d readers\n", w, (int) rw_active_readers);
            abort();
        }
    } else {
        ++rw_active_readers;
        if (rw_active_writers != 0) {
            fprintf(stderr, "VIOLATION: reader while a writer holds the lock\n");
            abort();
        }
    }
    if (rw_hold_us[writer] > 0)
        usleep(rw_hold_us[writer]);
}

// called by release, before it gives the lock up
static void rw_releasing(int writer) {
    if (writer) --rw_active_writers; else --rw_active_readers;
}

#endif
