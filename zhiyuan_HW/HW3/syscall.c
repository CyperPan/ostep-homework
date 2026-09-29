// Measure (1) the precision of the timers and (2) the cost of a null system call.
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define TIMER_SAMPLES 1000000
#define ITERATIONS    1000000

static double now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

// Smallest non-zero difference between back-to-back calls to gettimeofday().
static void gettimeofday_precision(void) {
    struct timeval a, b;
    long min_step = -1, zero = 0;
    for (int i = 0; i < TIMER_SAMPLES; i++) {
        gettimeofday(&a, NULL);
        gettimeofday(&b, NULL);
        long d = (b.tv_sec - a.tv_sec) * 1000000L + (b.tv_usec - a.tv_usec);
        if (d == 0)
            zero++;
        else if (min_step < 0 || d < min_step)
            min_step = d;
    }
    printf("gettimeofday: %ld of %d back-to-back pairs differ by 0 us; smallest non-zero step = %ld us\n",
           zero, TIMER_SAMPLES, min_step);
}

static void clock_gettime_precision(void) {
    struct timespec res, a, b;
    clock_getres(CLOCK_MONOTONIC, &res);
    long min_step = -1;
    for (int i = 0; i < TIMER_SAMPLES; i++) {
        clock_gettime(CLOCK_MONOTONIC, &a);
        clock_gettime(CLOCK_MONOTONIC, &b);
        long d = (b.tv_sec - a.tv_sec) * 1000000000L + (b.tv_nsec - a.tv_nsec);
        if (d > 0 && (min_step < 0 || d < min_step))
            min_step = d;
    }
    printf("clock_gettime(CLOCK_MONOTONIC): resolution %ld ns; smallest non-zero step = %ld ns\n",
           res.tv_nsec, min_step);
}

int main(int argc, char *argv[]) {
    gettimeofday_precision();
    clock_gettime_precision();

    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    char buf[1];

    // empty loop, so its cost can be subtracted
    double start = now_us();
    for (volatile int i = 0; i < ITERATIONS; i++)
        ;
    double loop = now_us() - start;

    start = now_us();
    for (volatile int i = 0; i < ITERATIONS; i++)
        if (read(fd, buf, 0) < 0) {
            perror("read");
            exit(1);
        }
    double elapsed = now_us() - start;

    printf("\n%d x read(fd, buf, 0): %.0f us total, loop overhead %.0f us\n",
           ITERATIONS, elapsed, loop);
    printf("cost of one system call: %.3f us (%.0f ns)\n",
           (elapsed - loop) / ITERATIONS, (elapsed - loop) / ITERATIONS * 1000);
    close(fd);
    return 0;
}
