#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

#include "main-header.h"
#include "vector-header.h"

int retry = 0;

void vector_add(vector_t *v_dst, vector_t *v_src) {
  top:
    // Variant for question 7 (vector-try-wait-block, built with -DBLOCK_FIRST):
    // wait for the first lock normally instead of spinning on trylock.
#ifdef BLOCK_FIRST
    Pthread_mutex_lock(&v_dst->lock);
#else
    if (pthread_mutex_trylock(&v_dst->lock) != 0) {
	goto top;
    }
#endif
    if (pthread_mutex_trylock(&v_src->lock) != 0) {
	retry++;
	Pthread_mutex_unlock(&v_dst->lock);
	goto top;
    }
    int i;
    for (i = 0; i < VECTOR_SIZE; i++) {
	v_dst->values[i] = v_dst->values[i] + v_src->values[i];
    }
    Pthread_mutex_unlock(&v_dst->lock);
    Pthread_mutex_unlock(&v_src->lock);
}

void fini() {
    printf("Retries: %d\n", retry);
}

#include "main-common.c"

