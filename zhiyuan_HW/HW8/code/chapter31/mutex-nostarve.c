#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include "common_threads.h"

//
// Here, you have to write (almost) ALL the code. Oh no!
// How can you show that a thread does not starve
// when attempting to acquire this mutex you build?
//
// Morris's algorithm (J. M. Morris, 1979; Downey, "A Little Book of
// Semaphores", 4.3). It only needs weak semaphores, i.e. a post may wake
// any waiting thread, which is what the Zemaphore in common_threads.h is.
//
// There are two waiting rooms. Arriving threads gather in room 1. Turnstile
// t1 lets them into room 2 one at a time. The last thread to move closes t1
// (it does not post t1) and opens t2. Then the threads in room 2 go through
// the critical section one at a time, and the last of them opens t1 again.
// While room 2 is being emptied, t1 is closed, so new arrivals (including a
// thread that just left the critical section and wants it again) wait in
// room 1 for the next round. So a waiting thread is overtaken by at most
// one round of other threads: at most N - 1 acquisitions in each of the two
// rounds it can be part of.
//

// --- the semaphores used in this file -------------------------------------
// Morris's algorithm needs a weak semaphore in Downey's sense (Property 4.1):
// "if there are threads waiting on a semaphore when a thread executes
// signal, then one of the waiting threads has to be woken". Which waiter is
// not specified. The Zemaphore of Figure 31.17 (sem_t on macOS here) is weaker
// than that: Zem_post() only increments the value, and a thread that calls
// Zem_wait() a moment later can take it before the signaled thread wakes up
// ("barging"). So this file has its own semaphore, xsem_t, in two kinds
// (-s option):
//   lifo  a weak semaphore with no barging, as in Property 4.1, but with the
//         worst choice of waiter: post hands the semaphore directly to the
//         thread that started waiting LAST. This is the default.
//   zem   the Zemaphore (barging allowed).
typedef struct __waiter_t {
    int granted;
    pthread_cond_t cv;
    struct __waiter_t *next;
} waiter_t;

typedef struct __xsem_t {
    int value;
    pthread_mutex_t lock;
    pthread_cond_t cond;   // zem: waiters wait here
    waiter_t *top;         // lifo: stack of waiters, newest first
} xsem_t;

int lifo = 1;  // which kind every xsem_t is

void xsem_init(xsem_t *s, int value) {
    s->value = value;
    s->top = NULL;
    Mutex_init(&s->lock);
    Cond_init(&s->cond);
}

void xsem_wait(xsem_t *s) {
    Mutex_lock(&s->lock);
    if (!lifo) {                         // Zemaphore
        while (s->value <= 0)
            Cond_wait(&s->cond, &s->lock);
        s->value--;
    } else if (s->value > 0) {           // lifo, nobody to wait for
        s->value--;
    } else {                             // lifo: wait until a post picks me
        waiter_t me = { .granted = 0, .next = s->top };
        Cond_init(&me.cv);
        s->top = &me;
        while (!me.granted)
            Cond_wait(&me.cv, &s->lock);
        pthread_cond_destroy(&me.cv);
    }
    Mutex_unlock(&s->lock);
}

void xsem_post(xsem_t *s) {
    Mutex_lock(&s->lock);
    if (lifo && s->top != NULL) {        // hand over to the newest waiter;
        waiter_t *w = s->top;            // value stays 0, so no one can barge
        s->top = w->next;
        w->granted = 1;
        Cond_signal(&w->cv);
    } else {
        s->value++;
        if (!lifo)
            Cond_signal(&s->cond);
    }
    Mutex_unlock(&s->lock);
}

// For the test below: the number of acquisitions so far, read when a thread
// arrives (see "overtakes" at the bottom of the file).
extern _Atomic long acquisitions;
static __thread long arrived_at;

typedef struct __ns_mutex_t {
    int room1, room2;  // number of threads in each waiting room
    xsem_t mutex;       // protects room1
    xsem_t t1, t2;      // turnstiles into room 2 and into the critical section
} ns_mutex_t;

void ns_mutex_init(ns_mutex_t *m) {
    m->room1 = m->room2 = 0;
    xsem_init(&m->mutex, 1);
    xsem_init(&m->t1, 1);
    xsem_init(&m->t2, 0);
}

void ns_mutex_acquire(ns_mutex_t *m) {
    xsem_wait(&m->mutex);
    m->room1++;
    arrived_at = acquisitions;  // test only: I am now waiting in room 1
    xsem_post(&m->mutex);

    xsem_wait(&m->t1);          // move from room 1 to room 2
    m->room2++;                // (only the holder of t1 changes room2 here)
    xsem_wait(&m->mutex);
    m->room1--;
    if (m->room1 == 0) {       // last one into room 2: close t1, open t2
        xsem_post(&m->mutex);
        xsem_post(&m->t2);
    } else {                   // let the next thread from room 1 in
        xsem_post(&m->mutex);
        xsem_post(&m->t1);
    }

    xsem_wait(&m->t2);          // into the critical section
    m->room2--;
}

void ns_mutex_release(ns_mutex_t *m) {
    // (braces needed: the Sem_post macro ends with its own semicolon)
    if (m->room2 == 0) {       // room 2 is empty: open t1 for the next round
        xsem_post(&m->t1);
    } else {                   // let the next thread of this round in
        xsem_post(&m->t2);
    }
}

//
// Test. Each of N threads acquires the lock `loops` times. For comparison,
// -m sem uses a plain binary semaphore (xsem_wait / xsem_post) as the lock.
//
// How to show that no thread starves: count, for every acquisition, how many
// times *other* threads got the lock while this thread was waiting
// ("overtakes"). If the lock is starvation-free, the largest such count stays
// bounded (here by 2(N - 1)) no matter how long the program runs. With a lock
// that allows starvation, it grows with the length of the run.
// "Waiting" starts when the thread arrives: for the plain semaphore that is
// the call of xsem_wait(), for Morris's lock it is the moment the thread is
// counted in room 1 (before that, the OS may simply not have run the thread
// yet, and no lock can bound that). max_wait_ms is measured from the call of
// the acquire function in both cases.
//
int use_ns = 1;
ns_mutex_t ns;
xsem_t plain;

int loops, num_threads;
long cs_work = 100, outside_work = 100;  // busy-loop iterations
_Atomic long acquisitions;                // total, incremented in the critical section
int inside = 0;                           // threads in the critical section (must be <= 1)
long counter = 0;                         // incremented without atomics, under the lock

typedef struct {
    long max_overtakes;
    long max_wait_ns;
    pthread_t tid;
    char pad[64];
} result_t;
result_t *results;

static long now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

static void spin(long n) {
    for (long i = 0; i < n; i++)
        __asm__ volatile("");
}

void *worker(void *arg) {
    result_t *r = arg;
    for (int i = 0; i < loops; i++) {
        arrived_at = atomic_load(&acquisitions);  // ns_mutex_acquire() sets it again
        long t0 = now_ns();
        if (use_ns) ns_mutex_acquire(&ns); else xsem_wait(&plain);
        long wait = now_ns() - t0;
        long overtakes = atomic_load(&acquisitions) - arrived_at;

        if (++inside != 1) { fprintf(stderr, "VIOLATION: %d threads inside\n", inside); abort(); }
        counter++;
        atomic_fetch_add(&acquisitions, 1);
        spin(cs_work);
        inside--;

        if (use_ns) ns_mutex_release(&ns); else xsem_post(&plain);

        if (overtakes > r->max_overtakes) r->max_overtakes = overtakes;
        if (wait > r->max_wait_ns) r->max_wait_ns = wait;
        spin(outside_work);
    }
    return NULL;
}

void usage(void) {
    fprintf(stderr, "usage: mutex-nostarve [-m ns|sem] [-s lifo|zem] [-c cs_work] [-o outside_work] threads loops\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    int c;
    while ((c = getopt(argc, argv, "m:s:c:o:")) != -1) {
        switch (c) {
        case 'm': use_ns = strcmp(optarg, "sem") != 0; break;
        case 's': lifo = strcmp(optarg, "zem") != 0; break;
        case 'c': cs_work = atol(optarg); break;
        case 'o': outside_work = atol(optarg); break;
        default: usage();
        }
    }
    if (argc - optind != 2) usage();
    num_threads = atoi(argv[optind]);
    loops = atoi(argv[optind + 1]);

    printf("parent: begin\n");
    ns_mutex_init(&ns);
    xsem_init(&plain, 1);
    results = calloc(num_threads, sizeof(result_t));

    long t0 = now_ns();
    for (int i = 0; i < num_threads; i++)
        Pthread_create(&results[i].tid, NULL, worker, &results[i]);
    for (int i = 0; i < num_threads; i++)
        Pthread_join(results[i].tid, NULL);
    double secs = (now_ns() - t0) / 1e9;

    long worst = 0, worst_wait = 0;
    for (int i = 0; i < num_threads; i++) {
        if (results[i].max_overtakes > worst) worst = results[i].max_overtakes;
        if (results[i].max_wait_ns > worst_wait) worst_wait = results[i].max_wait_ns;
    }
    printf("lock %s sem %s threads %d loops %d time %.3f max_overtakes %ld bound %d max_wait_ms %.2f counter %ld %s\n",
           use_ns ? "ns" : "sem", lifo ? "lifo" : "zem", num_threads, loops, secs, worst, 2 * (num_threads - 1),
           worst_wait / 1e6, counter, counter == (long) num_threads * loops ? "ok" : "WRONG");
    printf("parent: end\n");
    return 0;
}
