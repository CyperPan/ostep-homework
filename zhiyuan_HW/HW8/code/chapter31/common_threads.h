#ifndef __common_threads_h__
#define __common_threads_h__

#include <pthread.h>
#include <assert.h>
#include <sched.h>

#ifdef __linux__
#include <semaphore.h>
#endif

#define Pthread_create(thread, attr, start_routine, arg) assert(pthread_create(thread, attr, start_routine, arg) == 0);
#define Pthread_join(thread, value_ptr)                  assert(pthread_join(thread, value_ptr) == 0);

#define Pthread_mutex_init(m, v)                         assert(pthread_mutex_init(m, v) == 0);
#define Pthread_mutex_lock(m)                            assert(pthread_mutex_lock(m) == 0);
#define Pthread_mutex_unlock(m)                          assert(pthread_mutex_unlock(m) == 0);

#define Pthread_cond_init(cond, v)                       assert(pthread_cond_init(cond, v) == 0);
#define Pthread_cond_signal(cond)                        assert(pthread_cond_signal(cond) == 0);
#define Pthread_cond_wait(cond, mutex)                   assert(pthread_cond_wait(cond, mutex) == 0);

#define Mutex_init(m)                                    assert(pthread_mutex_init(m, NULL) == 0);
#define Mutex_lock(m)                                    assert(pthread_mutex_lock(m) == 0);
#define Mutex_unlock(m)                                  assert(pthread_mutex_unlock(m) == 0);

#define Cond_init(cond)                                  assert(pthread_cond_init(cond, NULL) == 0);
#define Cond_signal(cond)                                assert(pthread_cond_signal(cond) == 0);
#define Cond_wait(cond, mutex)                           assert(pthread_cond_wait(cond, mutex) == 0);

#ifdef __linux__
#define Sem_init(sem, value)                             assert(sem_init(sem, 0, value) == 0);
#define Sem_wait(sem)                                    assert(sem_wait(sem) == 0);
#define Sem_post(sem)                                    assert(sem_post(sem) == 0);
#endif // __linux__

// --- added for macOS (not in the original file) ------------------------------
// macOS declares sem_init() but does not implement unnamed semaphores: it
// returns -1 with errno ENOSYS ("Function not implemented"). So on macOS,
// sem_t is the "Zemaphore" of Figure 31.17: a value protected by a lock, and
// a condition variable to wait on. As in the book, the value never goes below
// zero, and which waiting thread a post wakes up is not specified.
#ifdef __APPLE__
typedef struct __zem_t {
    int value;
    pthread_cond_t cond;
    pthread_mutex_t lock;
} zem_t;

static inline void Zem_init(zem_t *s, int value) {
    s->value = value;
    Cond_init(&s->cond);
    Mutex_init(&s->lock);
}

static inline void Zem_wait(zem_t *s) {
    Mutex_lock(&s->lock);
    while (s->value <= 0)
        Cond_wait(&s->cond, &s->lock);
    s->value--;
    Mutex_unlock(&s->lock);
}

static inline void Zem_post(zem_t *s) {
    Mutex_lock(&s->lock);
    s->value++;
    Cond_signal(&s->cond);
    Mutex_unlock(&s->lock);
}

#define sem_t                                            zem_t
#define Sem_init(sem, value)                             Zem_init(sem, value);
#define Sem_wait(sem)                                    Zem_wait(sem);
#define Sem_post(sem)                                    Zem_post(sem);
#endif // __APPLE__

#endif // __common_threads_h__
