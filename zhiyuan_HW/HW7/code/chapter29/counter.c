// Concurrent counters (OSTEP chapter 29, Q2 and Q3).
//
// usage: ./counter [-s threshold] threads increments
//   Each of `threads` threads increments the counter `increments` times.
//   without -s: the simple counter, one lock around one integer (Figure 29.2)
//   -s S:       the approximate (sloppy) counter of Figure 29.4: one local
//               counter + lock per CPU, moved into the global counter every
//               S increments
// output: kind threads threshold increments seconds final expected ok
#include <string.h>
#include <unistd.h>
#include "common.h"

// --- simple counter ---------------------------------------------------------
typedef struct {
    int value;
    pthread_mutex_t lock;
} counter_t;

static void counter_init(counter_t *c) {
    c->value = 0;
    pthread_mutex_init(&c->lock, NULL);
}

static void counter_increment(counter_t *c) {
    Pthread_mutex_lock(&c->lock);
    c->value++;
    Pthread_mutex_unlock(&c->lock);
}

static int counter_get(counter_t *c) {
    Pthread_mutex_lock(&c->lock);
    int rc = c->value;
    Pthread_mutex_unlock(&c->lock);
    return rc;
}

// --- approximate counter ----------------------------------------------------
// Each local counter gets its own cache line, so threads on different CPUs
// do not slow each other down through false sharing.
typedef struct {
    int value;
    pthread_mutex_t lock;
} __attribute__((aligned(CACHE_LINE))) local_t;

typedef struct {
    int global;
    pthread_mutex_t glock;
    local_t *local;
    int ncpus;
    int threshold;
} approx_t;

static void approx_init(approx_t *c, int threshold, int ncpus) {
    c->threshold = threshold;
    c->ncpus = ncpus;
    c->global = 0;
    pthread_mutex_init(&c->glock, NULL);
    if (posix_memalign((void **) &c->local, CACHE_LINE, ncpus * sizeof(local_t)) != 0) {
        perror("posix_memalign");
        exit(1);
    }
    for (int i = 0; i < ncpus; i++) {
        c->local[i].value = 0;
        pthread_mutex_init(&c->local[i].lock, NULL);
    }
}

// thread tid uses local counter tid % ncpus, as in the chapter
static void approx_update(approx_t *c, int tid, int amt) {
    local_t *l = &c->local[tid % c->ncpus];
    Pthread_mutex_lock(&l->lock);
    l->value += amt;
    if (l->value >= c->threshold) {
        Pthread_mutex_lock(&c->glock);
        c->global += l->value;
        Pthread_mutex_unlock(&c->glock);
        l->value = 0;
    }
    Pthread_mutex_unlock(&l->lock);
}

// approximate: does not include what is still in the local counters
static int approx_get(approx_t *c) {
    Pthread_mutex_lock(&c->glock);
    int rc = c->global;
    Pthread_mutex_unlock(&c->glock);
    return rc;
}

// --- benchmark ----------------------------------------------------------------
static counter_t simple;
static approx_t approx;
static int use_approx;
static long increments;

typedef struct {
    int tid;
} arg_t;

static void *worker(void *p) {
    int tid = ((arg_t *) p)->tid;
    wait_for_start();
    if (use_approx)
        for (long i = 0; i < increments; i++)
            approx_update(&approx, tid, 1);
    else
        for (long i = 0; i < increments; i++)
            counter_increment(&simple);
    return NULL;
}

int main(int argc, char *argv[]) {
    int threshold = 0, opt;
    while ((opt = getopt(argc, argv, "s:")) != -1) {
        if (opt != 's')
            goto usage;
        threshold = atoi(optarg);
        use_approx = 1;
    }
    if (argc - optind != 2 || (use_approx && threshold < 1)) {
    usage:
        fprintf(stderr, "usage: %s [-s threshold] threads increments\n", argv[0]);
        exit(1);
    }
    int nthreads = atoi(argv[optind]);
    increments = atol(argv[optind + 1]);
    int ncpus = (int) sysconf(_SC_NPROCESSORS_ONLN);

    counter_init(&simple);
    approx_init(&approx, use_approx ? threshold : 1, ncpus);

    arg_t *args = malloc(nthreads * sizeof(arg_t));
    for (int i = 0; i < nthreads; i++)
        args[i].tid = i;
    double secs = run_threads(nthreads, worker, args, sizeof(arg_t));

    long expected = (long) nthreads * increments;
    long final;
    if (use_approx) {
        // the exact total is the global value plus whatever is still local
        final = approx_get(&approx);
        for (int i = 0; i < ncpus; i++)
            final += approx.local[i].value;
    } else {
        final = counter_get(&simple);
    }
    printf("%s %d %d %ld %.6f %ld %ld %s\n", use_approx ? "approx" : "simple", nthreads,
           threshold, increments, secs, final, expected, final == expected ? "ok" : "WRONG");
    free(args);
    return 0;
}
