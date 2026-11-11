// A concurrent hash table (OSTEP chapter 29, Q5 and Q6): a table of buckets,
// each a linked list of (key, value) nodes, with a configurable number of
// locks. Bucket b is protected by lock b % locks:
//   -l 1         one lock for the whole table (Q5, the simple strategy)
//   -l buckets   one lock per bucket, as in Figure 29.10 (Q6)
//   in between   lock striping: a fixed pool of locks shared by the buckets
//
// usage: ./hash [-l locks] [-u update%] threads ops buckets keys
//   The table starts with keys 0 .. keys-1, all with value 0. Each thread then
//   does `ops` operations on random keys: increment the key's value (with
//   probability update%, default 20) or read it. No keys are added, so the
//   chains keep the same length however many threads run.
// output: kind threads buckets keys locks update% ops seconds updates sum ok
#include <unistd.h>
#include "common.h"

typedef struct node {
    int key;
    long value;
    struct node *next;
} node_t;

// one lock per cache line, so neighbouring locks are not falsely shared
typedef struct {
    pthread_mutex_t m;
} __attribute__((aligned(CACHE_LINE))) lock_t;

static node_t **buckets;
static lock_t *locks;
static int nbuckets, nlocks;

static unsigned hash(int key) {
    return ((unsigned) key * 2654435761u) % (unsigned) nbuckets;
}

static void insert(int key) {  // only used to fill the table, single-threaded
    node_t *n = malloc(sizeof(node_t));
    n->key = key;
    n->value = 0;
    unsigned b = hash(key);
    n->next = buckets[b];
    buckets[b] = n;
}

// Find key; add `delta` to its value (0 = just read). Returns 1 if found.
static int access_key(int key, long delta, long *value) {
    unsigned b = hash(key);
    pthread_mutex_t *m = &locks[b % nlocks].m;
    int found = 0;
    Pthread_mutex_lock(m);
    for (node_t *n = buckets[b]; n != NULL; n = n->next)
        if (n->key == key) {
            n->value += delta;
            *value = n->value;
            found = 1;
            break;
        }
    Pthread_mutex_unlock(m);
    return found;
}

static long ops;
static int nkeys, update_pct;

typedef struct {
    int tid;
    long updates;
    long missing;
} __attribute__((aligned(CACHE_LINE))) arg_t;

static void *worker(void *p) {
    arg_t *a = p;
    unsigned seed = 777 + a->tid;
    long value;
    wait_for_start();
    for (long i = 0; i < ops; i++) {
        int key = rand_r(&seed) % nkeys;
        int update = (int) (rand_r(&seed) % 100) < update_pct;
        if (!access_key(key, update ? 1 : 0, &value))
            a->missing++;
        a->updates += update;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int opt;
    nlocks = 1;
    update_pct = 20;
    while ((opt = getopt(argc, argv, "l:u:")) != -1) {
        switch (opt) {
        case 'l': nlocks = atoi(optarg); break;
        case 'u': update_pct = atoi(optarg); break;
        default:  goto usage;
        }
    }
    if (argc - optind != 4) {
    usage:
        fprintf(stderr, "usage: %s [-l locks] [-u update%%] threads ops buckets keys\n", argv[0]);
        exit(1);
    }
    int nthreads = atoi(argv[optind]);
    ops = atol(argv[optind + 1]);
    nbuckets = atoi(argv[optind + 2]);
    nkeys = atoi(argv[optind + 3]);
    if (nlocks < 1 || nlocks > nbuckets) {
        fprintf(stderr, "locks must be between 1 and the number of buckets\n");
        exit(1);
    }

    buckets = calloc(nbuckets, sizeof(node_t *));
    if (posix_memalign((void **) &locks, CACHE_LINE, nlocks * sizeof(lock_t)) != 0) {
        perror("posix_memalign");
        exit(1);
    }
    for (int i = 0; i < nlocks; i++)
        pthread_mutex_init(&locks[i].m, NULL);
    for (int k = 0; k < nkeys; k++)
        insert(k);

    arg_t *args = calloc(nthreads, sizeof(arg_t));
    for (int i = 0; i < nthreads; i++)
        args[i].tid = i;
    double secs = run_threads(nthreads, worker, args, sizeof(arg_t));

    // every key exists, and the values must add up to the number of updates
    long updates = 0, missing = 0, sum = 0;
    for (int i = 0; i < nthreads; i++) {
        updates += args[i].updates;
        missing += args[i].missing;
    }
    for (int b = 0; b < nbuckets; b++)
        for (node_t *n = buckets[b]; n != NULL; n = n->next)
            sum += n->value;
    const char *kind = nlocks == 1 ? "global" : nlocks == nbuckets ? "per-bucket" : "striped";
    printf("%s %d %d %d %d %d %ld %.6f %ld %ld %s\n", kind, nthreads, nbuckets, nkeys, nlocks,
           update_pct, ops, secs, updates, sum, missing == 0 && sum == updates ? "ok" : "WRONG");
    free(args);
    return 0;
}
