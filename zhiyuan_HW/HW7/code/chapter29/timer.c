// How precise and how expensive are the timers? (OSTEP chapter 29, Q1)
// For each timer: call it back to back many times, record the smallest
// non-zero step (resolution) and the average cost of one call.
//
// rdtsc is x86 only. On ARM the equivalent is the generic timer's counter
// register cntvct_el0, which Linux lets user code read directly. macOS does
// not give user code a usable cntvct_el0 (reads return garbage); its
// interface to the same hardware counter is mach_absolute_time().
//
// All times are 64-bit integer nanoseconds: a double cannot hold today's
// time in ns (~1.7e18) to better than 256 ns, which would hide the real
// resolution.
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#define CALLS 1000000

static uint64_t tv_ns(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t) tv.tv_sec * 1000000000u + (uint64_t) tv.tv_usec * 1000u;
}

static uint64_t ts_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000000000u + (uint64_t) ts.tv_nsec;
}

#ifdef __APPLE__
static mach_timebase_info_data_t tb;

static uint64_t mach_ns(void) {
    return mach_absolute_time() * tb.numer / tb.denom;
}
#endif

#if defined(__linux__) && defined(__aarch64__)
#define HAVE_COUNTER "cntvct_el0 (ARM's rdtsc)"
static uint64_t freq;

static inline uint64_t read_counter(void) {
    uint64_t v;
    __asm__ volatile("isb; mrs %0, cntvct_el0" : "=r"(v));
    return v;
}

static uint64_t counter_ns(void) {
    return read_counter() * 1000000000u / freq;
}
#elif defined(__x86_64__)
#define HAVE_COUNTER "rdtsc (cycles, not ns)"
#include <x86intrin.h>
static uint64_t counter_ns(void) {
    return __rdtsc();
}
#endif

static void measure(const char *name, uint64_t (*now)(void)) {
    uint64_t min_step = UINT64_MAX;
    uint64_t start = now();
    for (int i = 0; i < CALLS; i++) {
        uint64_t a = now();
        uint64_t b = now();
        if (b > a && b - a < min_step)
            min_step = b - a;
    }
    double per_call = (double) (now() - start) / (2.0 * CALLS);
    printf("%-34s smallest step %7llu ns   cost per call %5.1f ns\n", name,
           (unsigned long long) min_step, per_call);
}

int main(void) {
    struct timespec res;
    clock_getres(CLOCK_MONOTONIC, &res);
    measure("gettimeofday()", tv_ns);
    measure("clock_gettime(CLOCK_MONOTONIC)", ts_ns);
    printf("%-34s (clock_getres says %ld ns)\n", "", res.tv_nsec);
#ifdef __APPLE__
    mach_timebase_info(&tb);
    measure("mach_absolute_time()", mach_ns);
    printf("%-34s (one tick = %u/%u ns)\n", "", tb.numer, tb.denom);
#endif
#ifdef HAVE_COUNTER
#if defined(__linux__) && defined(__aarch64__)
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
    measure(HAVE_COUNTER, counter_ns);
    printf("%-34s (counter frequency %.1f MHz)\n", "", freq / 1e6);
#else
    measure(HAVE_COUNTER, counter_ns);
#endif
#endif
    return 0;
}
