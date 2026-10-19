// Q8: a growable vector built on realloc(), compared with a linked list.
//
// usage: ./vector [n]   (default n = 10000000)
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int *data;
    size_t size;
    size_t capacity;
} vector_t;

static void vector_init(vector_t *v) {
    v->data = NULL;
    v->size = 0;
    v->capacity = 0;
}

static void vector_push(vector_t *v, int x) {
    if (v->size == v->capacity) {
        size_t cap = v->capacity ? 2 * v->capacity : 4;     // double when full
        int *p = realloc(v->data, cap * sizeof(int));
        if (p == NULL) {
            perror("realloc");
            exit(1);
        }
        v->data = p;
        v->capacity = cap;
    }
    v->data[v->size++] = x;
}

static void vector_free(vector_t *v) {
    free(v->data);
    vector_init(v);
}

typedef struct node {
    int value;
    struct node *next;
} node_t;

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

int main(int argc, char *argv[]) {
    size_t n = argc > 1 ? strtoul(argv[1], NULL, 10) : 10000000;
    long sum;

    // vector: append n ints, then sum them
    vector_t v;
    vector_init(&v);
    double t0 = now_ms();
    size_t reallocs = 0, last = 0;
    for (size_t i = 0; i < n; i++) {
        vector_push(&v, (int) i);
        if (v.capacity != last) {
            reallocs++;
            last = v.capacity;
        }
    }
    double t1 = now_ms();
    sum = 0;
    for (size_t i = 0; i < v.size; i++)
        sum += v.data[i];
    double t2 = now_ms();
    printf("vector:      append %.1f ms (%zu reallocs, capacity %zu), traverse %.1f ms, sum %ld, %zu bytes\n",
           t1 - t0, reallocs, v.capacity, t2 - t1, sum, v.capacity * sizeof(int));
    vector_free(&v);

    // linked list: append n ints (keeping a tail pointer), then sum them
    node_t *head = NULL, *tail = NULL;
    t0 = now_ms();
    for (size_t i = 0; i < n; i++) {
        node_t *nd = malloc(sizeof(node_t));
        if (nd == NULL) {
            perror("malloc");
            exit(1);
        }
        nd->value = (int) i;
        nd->next = NULL;
        if (tail)
            tail->next = nd;
        else
            head = nd;
        tail = nd;
    }
    t1 = now_ms();
    sum = 0;
    for (node_t *p = head; p; p = p->next)
        sum += p->value;
    t2 = now_ms();
    printf("linked list: append %.1f ms (%zu mallocs), traverse %.1f ms, sum %ld, %zu bytes of nodes\n",
           t1 - t0, n, t2 - t1, sum, n * sizeof(node_t));
    while (head) {
        node_t *next = head->next;
        free(head);
        head = next;
    }
    return 0;
}
