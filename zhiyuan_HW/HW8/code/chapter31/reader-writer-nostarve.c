#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common_threads.h"

//
// Your code goes in the structure and functions below
//

#include "rw_test.h"

// The lock of reader-writer.c plus a turnstile (Downey, "A Little Book of
// Semaphores", 4.2.3). Every thread passes the turnstile on the way in. A
// writer keeps the turnstile closed while it waits for the readers inside to
// leave, so no new reader can join them. When the writer is done it opens the
// turnstile again and the threads that queued up behind it go in.
//
// The turnstile must be a *strong* (FIFO) semaphore. With a weak one (here
// the Zemaphore, built with -DWEAK_TURNSTILE), a writer that leaves and comes
// straight back can take the turnstile again before a reader that has waited
// on it for a long time; with several writers the reader then starves.
// fifo_sem_t serves its waiters strictly in the order they called wait.

typedef struct __fifo_sem_t {
    int value;
    unsigned long next_ticket, now_serving;  // a ticket queue (as in a ticket lock)
    pthread_mutex_t lock;
    pthread_cond_t cond;
} fifo_sem_t;

void fifo_sem_init(fifo_sem_t *s, int value) {
    s->value = value;
    s->next_ticket = s->now_serving = 0;
    Mutex_init(&s->lock);
    Cond_init(&s->cond);
}

void fifo_sem_wait(fifo_sem_t *s) {
    Mutex_lock(&s->lock);
    unsigned long me = s->next_ticket++;
    while (me != s->now_serving || s->value <= 0)  // my turn, and something to take
        Cond_wait(&s->cond, &s->lock);
    s->value--;
    s->now_serving++;
    pthread_cond_broadcast(&s->cond);              // the next ticket may go now
    Mutex_unlock(&s->lock);
}

void fifo_sem_post(fifo_sem_t *s) {
    Mutex_lock(&s->lock);
    s->value++;
    pthread_cond_broadcast(&s->cond);
    Mutex_unlock(&s->lock);
}

#ifdef WEAK_TURNSTILE
#define turnstile_t     sem_t
#define T_init(t, v)    Sem_init(t, v)
#define T_wait(t)       Sem_wait(t)
#define T_post(t)       Sem_post(t)
#else
#define turnstile_t     fifo_sem_t
#define T_init(t, v)    fifo_sem_init(t, v);
#define T_wait(t)       fifo_sem_wait(t);
#define T_post(t)       fifo_sem_post(t);
#endif

typedef struct __rwlock_t {
    sem_t lock;             // binary semaphore: protects readers
    sem_t writelock;        // held by one writer, or by the group of readers
    turnstile_t turnstile;  // closed by a writer that is waiting or writing
    int readers;
} rwlock_t;


void rwlock_init(rwlock_t *rw) {
    rw->readers = 0;
    Sem_init(&rw->lock, 1);
    Sem_init(&rw->writelock, 1);
    T_init(&rw->turnstile, 1);
    rw_test_init();
}

void rwlock_acquire_readlock(rwlock_t *rw) {
    long start = rw_now_ns();
    T_wait(&rw->turnstile);      // blocks while a writer is waiting
    T_post(&rw->turnstile);
    Sem_wait(&rw->lock);
    rw->readers++;
    if (rw->readers == 1)
        Sem_wait(&rw->writelock);
    Sem_post(&rw->lock);
    rw_got_lock(0, start);
}

void rwlock_release_readlock(rwlock_t *rw) {
    rw_releasing(0);
    Sem_wait(&rw->lock);
    rw->readers--;
    if (rw->readers == 0)
        Sem_post(&rw->writelock);
    Sem_post(&rw->lock);
}

void rwlock_acquire_writelock(rwlock_t *rw) {
    long start = rw_now_ns();
    T_wait(&rw->turnstile);      // close the turnstile: no new readers
    Sem_wait(&rw->writelock);    // wait for the readers inside to leave
    rw_got_lock(1, start);
}

void rwlock_release_writelock(rwlock_t *rw) {
    rw_releasing(1);
    T_post(&rw->turnstile);      // open the turnstile first ...
    Sem_post(&rw->writelock);    // ... then the room
}

//
// Don't change the code below (just use it!)
// 

int loops;
int value = 0;

rwlock_t lock;

void *reader(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
	rwlock_acquire_readlock(&lock);
	printf("read %d\n", value);
	rwlock_release_readlock(&lock);
    }
    return NULL;
}

void *writer(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
	rwlock_acquire_writelock(&lock);
	value++;
	printf("write %d\n", value);
	rwlock_release_writelock(&lock);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    assert(argc == 4);
    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);
    loops = atoi(argv[3]);

    pthread_t pr[num_readers], pw[num_writers];

    rwlock_init(&lock);

    printf("begin\n");

    int i;
    for (i = 0; i < num_readers; i++)
	Pthread_create(&pr[i], NULL, reader, NULL);
    for (i = 0; i < num_writers; i++)
	Pthread_create(&pw[i], NULL, writer, NULL);

    for (i = 0; i < num_readers; i++)
	Pthread_join(pr[i], NULL);
    for (i = 0; i < num_writers; i++)
	Pthread_join(pw[i], NULL);

    printf("end: value %d\n", value);

    return 0;
}

