// Concurrent linked lists (OSTEP chapter 29, Q4): one lock for the whole list
// (Figure 29.8) versus hand-over-hand locking (one lock per node).
//
// usage: ./list [-H] [-w work] [-i insert%] threads ops length
//   The list starts with keys 0 .. length-1. Each thread then does `ops`
//   operations: a lookup of a random existing key, or (with probability
//   insert%) an insert of a new key at the head.
//   -H       hand-over-hand locking (default: one lock for the whole list)
//   -w work  busy-loop iterations per node visited, standing in for an
//            expensive key comparison (default 0)
// output: kind threads length ops work insert% seconds found expected size ok
#include <unistd.h>
#include "common.h"

typedef struct node {
    int key;
    struct node *next;
    pthread_mutex_t lock;  // only used by hand-over-hand
} node_t;

typedef struct {
    node_t head;           // sentinel; head.lock guards head.next
    pthread_mutex_t lock;  // only used by the single-lock list
} list_t;

static list_t list;
static int hoh;
static long work;

static node_t *new_node(int key) {
    node_t *n = malloc(sizeof(node_t));
    if (n == NULL) {
        perror("malloc");
        exit(1);
    }
    n->key = key;
    pthread_mutex_init(&n->lock, NULL);
    return n;
}

// --- one lock for the whole list -----------------------------------------------
static void single_insert(int key) {
    node_t *n = new_node(key);
    Pthread_mutex_lock(&list.lock);
    n->next = list.head.next;
    list.head.next = n;
    Pthread_mutex_unlock(&list.lock);
}

static int single_lookup(int key) {
    int found = 0;
    Pthread_mutex_lock(&list.lock);
    for (node_t *cur = list.head.next; cur != NULL; cur = cur->next) {
        busy_work(work);
        if (cur->key == key) {
            found = 1;
            break;
        }
    }
    Pthread_mutex_unlock(&list.lock);
    return found;
}

// --- hand-over-hand ----------------------------------------------------------
// Inserting at the head only needs the sentinel's lock: every traversal
// starts by taking that lock, so it never sees a half-linked node.
static void hoh_insert(int key) {
    node_t *n = new_node(key);
    Pthread_mutex_lock(&list.head.lock);
    n->next = list.head.next;
    list.head.next = n;
    Pthread_mutex_unlock(&list.head.lock);
}

// Hold the current node's lock while taking the next node's lock, then
// release the current one: a thread never holds more than two locks, and
// another thread can work on the nodes it has already passed.
static int hoh_lookup(int key) {
    int found = 0;
    node_t *prev = &list.head;
    Pthread_mutex_lock(&prev->lock);
    node_t *cur = prev->next;
    while (cur != NULL) {
        Pthread_mutex_lock(&cur->lock);
        Pthread_mutex_unlock(&prev->lock);
        busy_work(work);
        if (cur->key == key) {
            found = 1;
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    Pthread_mutex_unlock(cur != NULL ? &cur->lock : &prev->lock);
    return found;
}

// --- benchmark ---------------------------------------------------------------
static long ops, length;
static int insert_pct;

typedef struct {
    int tid;
    long found;
    long lookups;
} __attribute__((aligned(CACHE_LINE))) arg_t;

static void *worker(void *p) {
    arg_t *a = p;
    unsigned seed = 12345 + a->tid;
    int next_key = (int) (length + a->tid * ops);  // keys no other thread uses
    wait_for_start();
    for (long i = 0; i < ops; i++) {
        if ((int) (rand_r(&seed) % 100) < insert_pct) {
            if (hoh)
                hoh_insert(next_key++);
            else
                single_insert(next_key++);
        } else {
            int key = rand_r(&seed) % length;
            a->found += hoh ? hoh_lookup(key) : single_lookup(key);
            a->lookups++;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "Hw:i:")) != -1) {
        switch (opt) {
        case 'H': hoh = 1; break;
        case 'w': work = atol(optarg); break;
        case 'i': insert_pct = atoi(optarg); break;
        default:  goto usage;
        }
    }
    if (argc - optind != 3) {
    usage:
        fprintf(stderr, "usage: %s [-H] [-w work] [-i insert%%] threads ops length\n", argv[0]);
        exit(1);
    }
    int nthreads = atoi(argv[optind]);
    ops = atol(argv[optind + 1]);
    length = atol(argv[optind + 2]);

    list.head.next = NULL;
    pthread_mutex_init(&list.head.lock, NULL);
    pthread_mutex_init(&list.lock, NULL);
    for (long k = 0; k < length; k++)
        single_insert((int) k);

    arg_t *args = calloc(nthreads, sizeof(arg_t));
    for (int i = 0; i < nthreads; i++)
        args[i].tid = i;
    double secs = run_threads(nthreads, worker, args, sizeof(arg_t));

    // every lookup was for a key that is in the list, so all must be found,
    // and the list must contain the initial keys plus every insert
    long found = 0, lookups = 0, size = 0;
    for (int i = 0; i < nthreads; i++) {
        found += args[i].found;
        lookups += args[i].lookups;
    }
    for (node_t *n = list.head.next; n != NULL; n = n->next)
        size++;
    long inserts = (long) nthreads * ops - lookups;
    int ok = found == lookups && size == length + inserts;
    printf("%s %d %ld %ld %ld %d %.6f %ld %ld %ld %s\n", hoh ? "hoh" : "single", nthreads, length,
           ops, work, insert_pct, secs, found, lookups, size, ok ? "ok" : "WRONG");
    free(args);
    return 0;
}
