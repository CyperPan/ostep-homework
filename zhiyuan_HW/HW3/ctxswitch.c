// Measure the cost of a context switch, lmbench style: two processes pinned to
// the same CPU pass a byte back and forth over two pipes.
//
// usage: ./ctxswitch [same|diff]   (default: same CPU)
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ROUNDS 200000

static double now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static void pin_to_cpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set) < 0)
        die("sched_setaffinity");
}

int main(int argc, char *argv[]) {
    int same_cpu = !(argc > 1 && strcmp(argv[1], "diff") == 0);
    int p1[2], p2[2]; // p1: parent -> child, p2: child -> parent
    if (pipe(p1) < 0 || pipe(p2) < 0) {
        perror("pipe");
        exit(1);
    }
    char c = 'x';

    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        pin_to_cpu(same_cpu ? 0 : 1);
        for (int i = 0; i < ROUNDS; i++) {
            if (read(p1[0], &c, 1) != 1 ||  // block until the parent writes
                write(p2[1], &c, 1) != 1)   // wake the parent
                die("child read/write");
        }
        exit(0);
    }

    pin_to_cpu(0);
    double start = now_us();
    for (int i = 0; i < ROUNDS; i++) {
        if (write(p1[1], &c, 1) != 1 ||  // wake the child
            read(p2[0], &c, 1) != 1)     // block until the child answers
            die("parent read/write");
    }
    double elapsed = now_us() - start;
    wait(NULL);

    // each round trip = 2 context switches (parent -> child -> parent)
    printf("%s CPU: %d round trips in %.0f us\n",
           same_cpu ? "same" : "different", ROUNDS, elapsed);
    printf("  per round trip: %.3f us\n", elapsed / ROUNDS);
    printf("  per context switch (incl. one pipe write + read): %.3f us\n",
           elapsed / ROUNDS / 2);
    return 0;
}
