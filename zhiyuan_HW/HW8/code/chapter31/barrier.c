#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "common_threads.h"

// If done correctly, each child should print their "before" message
// before either prints their "after" message. Test by adding sleep(1)
// calls in various locations.

// You likely need two semaphores to do this correctly, and some
// other integers to track things.

typedef struct __barrier_t {
    // add semaphores and other information here
    int num_threads;   // N: how many threads must arrive
    int count;         // how many have arrived so far
    sem_t mutex;       // binary semaphore (a lock) protecting count
    sem_t turnstile;   // threads wait here until the last one arrives
} barrier_t;


// the single barrier we are using for this program
barrier_t b;

// Added for testing: if the environment variable BARRIER_SLEEP_US is set, each
// thread first sleeps a random time up to that many microseconds, so the
// threads reach the barrier at different times and in a random order.
static void test_delay(void) {
    char *s = getenv("BARRIER_SLEEP_US");
    if (s != NULL && atoi(s) > 0)
        usleep(random() % atoi(s));
}

void barrier_init(barrier_t *b, int num_threads) {
    // initialization code goes here
    srandom(time(NULL) ^ getpid());  // for test_delay()
    b->num_threads = num_threads;
    b->count = 0;
    Sem_init(&b->mutex, 1);
    Sem_init(&b->turnstile, 0);
}

void barrier(barrier_t *b) {
    // barrier code goes here
    test_delay();
    Sem_wait(&b->mutex);
    b->count++;
    int last = (b->count == b->num_threads);
    Sem_post(&b->mutex);

    if (last)                    // the N-th thread opens the turnstile ...
        Sem_post(&b->turnstile);
#ifndef BROKEN   // -DBROKEN removes the wait, to check that the test notices
    Sem_wait(&b->turnstile);     // everyone (the N-th one too) passes it ...
    Sem_post(&b->turnstile);     // ... and lets the next one through
#endif
}

//
// XXX: don't change below here (just run it!)
//
typedef struct __tinfo_t {
    int thread_id;
} tinfo_t;

void *child(void *arg) {
    tinfo_t *t = (tinfo_t *) arg;
    printf("child %d: before\n", t->thread_id);
    barrier(&b);
    printf("child %d: after\n", t->thread_id);
    return NULL;
}


// run with a single argument indicating the number of 
// threads you wish to create (1 or more)
int main(int argc, char *argv[]) {
    assert(argc == 2);
    int num_threads = atoi(argv[1]);
    assert(num_threads > 0);

    pthread_t p[num_threads];
    tinfo_t t[num_threads];

    printf("parent: begin\n");
    barrier_init(&b, num_threads);
    
    int i;
    for (i = 0; i < num_threads; i++) {
	t[i].thread_id = i;
	Pthread_create(&p[i], NULL, child, &t[i]);
    }

    for (i = 0; i < num_threads; i++) 
	Pthread_join(p[i], NULL);

    printf("parent: end\n");
    return 0;
}

