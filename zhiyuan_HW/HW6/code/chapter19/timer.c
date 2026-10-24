// How precise are gettimeofday() and clock_gettime()? (OSTEP chapter 19, Q1)
// Call each timer back to back and record the smallest non-zero step.
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#define CALLS 1000000

int main(void) {
    struct timeval tv, tv2;
    struct timespec ts, ts2, res;
    double min_tv = 1e9, min_ts = 1e9;

    for (int i = 0; i < CALLS; i++) {
        gettimeofday(&tv, NULL);
        gettimeofday(&tv2, NULL);
        double d = (tv2.tv_sec - tv.tv_sec) * 1e9 + (tv2.tv_usec - tv.tv_usec) * 1e3;
        if (d > 0 && d < min_tv)
            min_tv = d;
    }
    for (int i = 0; i < CALLS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        clock_gettime(CLOCK_MONOTONIC, &ts2);
        double d = (ts2.tv_sec - ts.tv_sec) * 1e9 + (ts2.tv_nsec - ts.tv_nsec);
        if (d > 0 && d < min_ts)
            min_ts = d;
    }
    clock_getres(CLOCK_MONOTONIC, &res);
    printf("gettimeofday:  smallest step %.0f ns\n", min_tv);
    printf("clock_gettime: smallest step %.0f ns (clock_getres says %ld ns)\n", min_ts, res.tv_nsec);
    return 0;
}
