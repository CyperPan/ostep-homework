// Measure the average cost of one memory access when touching NUMPAGES pages
// over and over (OSTEP chapter 19). Once the pages no longer fit in the TLB,
// the time per access jumps.
//
// usage: ./tlb [-c cpu] [-n] [-o] numpages trials
//   -c cpu  pin the process to this CPU (Linux only; -1 = do not pin, default 0)
//   -n      do NOT touch the array before timing (shows the demand-zero cost, Q7)
//   -o      use a different cache line in each page (page i touches line
//           i % lines-per-page), so the accesses do not all fall into the same
//           cache set; without it we partly measure cache conflicts, not the TLB
// output: numpages trials ns_per_access checksum
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

static void pin_to_cpu(int cpu) {
#ifdef __linux__
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set) < 0) {
        perror("sched_setaffinity");
        exit(1);
    }
#else
    (void) cpu; // macOS has no way to pin a thread to a core
#endif
}

int main(int argc, char *argv[]) {
    int cpu = 0, init = 1, stagger = 0, opt;
    while ((opt = getopt(argc, argv, "c:no")) != -1) {
        switch (opt) {
        case 'c': cpu = atoi(optarg); break;
        case 'n': init = 0; break;
        case 'o': stagger = 1; break;
        default:  goto usage;
        }
    }
    if (argc - optind != 2) {
    usage:
        fprintf(stderr, "usage: %s [-c cpu] [-n] [-o] numpages trials\n", argv[0]);
        exit(1);
    }
    long numpages = atol(argv[optind]);
    long trials   = atol(argv[optind + 1]);
    if (numpages < 1 || trials < 1)
        goto usage;
    if (cpu >= 0)
        pin_to_cpu(cpu);

    long pagesize = sysconf(_SC_PAGESIZE);
    long jump = pagesize / sizeof(int); // ints per page: a[i * jump] is on page i
    long lines = pagesize / 64;         // 64-byte cache lines per page
    int *a = malloc(numpages * pagesize);
    if (a == NULL) {
        perror("malloc");
        exit(1);
    }
    // idx[i] = which int of the array we touch on page i
    long *idx = malloc(numpages * sizeof(long));
    for (long i = 0; i < numpages; i++)
        idx[i] = i * jump + (stagger ? (i % lines) * (64 / sizeof(int)) : 0);
    if (init) // fault every page in now, so the timed loop does not pay for it
        for (long i = 0; i < numpages; i++)
            a[idx[i]] = 0;

#ifndef NAIVE
    double start = now_ns();
    for (long t = 0; t < trials; t++)
        for (long i = 0; i < numpages; i++)
            a[idx[i]] += 1;
    double end = now_ns();

    // Read the array back and print the result, so the compiler cannot
    // throw the timed loop away (Q5).
    long sum = 0;
    for (long i = 0; i < numpages; i++)
        sum += a[idx[i]];
    printf("%ld %ld %.3f %ld\n", numpages, trials, (end - start) / (numpages * trials), sum);
#else
    // Q5, the wrong way: only read the array and never use the result.
    // With -O2 the compiler sees the loop has no effect and deletes it.
    long unused = 0;
    double start = now_ns();
    for (long t = 0; t < trials; t++)
        for (long i = 0; i < numpages; i++)
            unused += a[idx[i]];
    double end = now_ns();
    printf("%ld %ld %.3f\n", numpages, trials, (end - start) / (numpages * trials));
#endif
    free(idx);
    free(a);
    return 0;
}
