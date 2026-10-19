// Q3: allocate N megabytes and keep touching every byte of it.
//
// usage: ./memory-user <megabytes> [seconds]   (runs forever if no time is given)
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s <megabytes> [seconds]\n", argv[0]);
        exit(1);
    }
    long mb = atol(argv[1]);
    int seconds = (argc == 3) ? atoi(argv[2]) : -1;
    size_t bytes = (size_t) mb * 1024 * 1024;

    char *array = malloc(bytes);
    if (array == NULL) {
        perror("malloc");
        exit(1);
    }
    printf("pid %d: using %ld MB (%zu bytes)%s\n", (int) getpid(), mb, bytes,
           seconds < 0 ? ", running until killed" : "");
    fflush(stdout);

    time_t end = time(NULL) + seconds;
    long passes = 0;
    while (seconds < 0 || time(NULL) < end) {
        for (size_t i = 0; i < bytes; i += 4096)   // one write per page is enough
            array[i] = (char) passes;              // to keep every page resident
        passes++;
    }
    printf("pid %d: done after %ld passes over the array\n", (int) getpid(), passes);
    free(array);
    return 0;
}
