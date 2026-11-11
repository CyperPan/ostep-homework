// Helpers shared by the chapter 29 programs: a timer, thread start/join and
// a busy loop that stands in for "work".
#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Apple M-series cores have 128-byte cache lines (x86: 64). Padding per-thread
// data to this size keeps two threads from sharing a line (false sharing).
#define CACHE_LINE 128

static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static inline void Pthread_mutex_lock(pthread_mutex_t *m) {
    if (pthread_mutex_lock(m) != 0) { perror("pthread_mutex_lock"); exit(1); }
}

static inline void Pthread_mutex_unlock(pthread_mutex_t *m) {
    if (pthread_mutex_unlock(m) != 0) { perror("pthread_mutex_unlock"); exit(1); }
}

// A start gate: worker threads call wait_for_start() and block until the main
// thread opens the gate, so thread creation is not part of the measured time.
// (macOS has no pthread_barrier_t, so this is a mutex + condition variable.)
static pthread_mutex_t gate_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_cond = PTHREAD_COND_INITIALIZER;
static int gate_open = 0;

static inline void wait_for_start(void) {
    Pthread_mutex_lock(&gate_lock);
    while (!gate_open)
        pthread_cond_wait(&gate_cond, &gate_lock);
    Pthread_mutex_unlock(&gate_lock);
}

// Start nthreads threads running fn(args + i * argsize), open the gate, and
// wait for all of them. Returns the elapsed wall-clock time in seconds.
static inline double run_threads(int nthreads, void *(*fn)(void *), void *args, size_t argsize) {
    pthread_t *tids = malloc(nthreads * sizeof(pthread_t));
    gate_open = 0;
    for (int i = 0; i < nthreads; i++)
        if (pthread_create(&tids[i], NULL, fn, (char *) args + i * argsize) != 0) {
            perror("pthread_create");
            exit(1);
        }
    Pthread_mutex_lock(&gate_lock);
    gate_open = 1;
    double start = now_sec();
    pthread_cond_broadcast(&gate_cond);
    Pthread_mutex_unlock(&gate_lock);
    for (int i = 0; i < nthreads; i++)
        pthread_join(tids[i], NULL);
    double end = now_sec();
    free(tids);
    return end - start;
}

// A loop the compiler cannot remove: simulates `iters` units of work. The
// empty asm statement keeps the loop; the counter stays in a register. (A
// `volatile` counter would go through memory on every iteration, and its
// speed then changed by 3x depending on the code around it.)
static inline void busy_work(long iters) {
    for (long i = 0; i < iters; i++)
        __asm__ volatile("");
}

#endif
