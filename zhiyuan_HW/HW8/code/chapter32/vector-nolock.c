#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "common_threads.h"

#include "main-header.h"
#include "vector-header.h"

// taken from https://en.wikipedia.org/wiki/Fetch-and-add
// (changed: the inline assembly is x86 only, and this Mac has an ARM CPU; on
// other CPUs the compiler's atomic builtin does the same thing; on Apple M4
// it compiles to the single atomic instruction LDADDAL)
int fetch_and_add(int * variable, int value) {
#if defined(__x86_64__) || defined(__i386__)
    asm volatile("lock; xaddl %%eax, %2;"
		 :"=a" (value)                  
		 :"a" (value), "m" (*variable)  
		 :"memory");
    return value;
#else
    return __atomic_fetch_add(variable, value, __ATOMIC_SEQ_CST);
#endif
}

void vector_add(vector_t *v_dst, vector_t *v_src) {
    int i;
    for (i = 0; i < VECTOR_SIZE; i++) {
	fetch_and_add(&v_dst->values[i], v_src->values[i]);
    }
}

void fini() {}


#include "main-common.c"

