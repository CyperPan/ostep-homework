#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common_threads.h"

// If done correctly, each child should print their "before" message
// before either prints their "after" message. Test by adding sleep(1)
// calls in various locations.

// Added for testing: "./rendezvous 1" makes child 1 sleep for a second before
// it reaches the rendezvous, "./rendezvous 2" does the same for child 2.
int sleeper = 0;

sem_t s1, s2;  // s1: "child 1 has arrived", s2: "child 2 has arrived"

void *child_1(void *arg) {
    if (sleeper == 1) sleep(1);
    printf("child 1: before\n");
    // what goes here?
    Sem_post(&s1);  // announce my arrival first ...
    Sem_wait(&s2);  // ... then wait for child 2's
    printf("child 1: after\n");
    return NULL;
}

void *child_2(void *arg) {
    if (sleeper == 2) sleep(1);
    printf("child 2: before\n");
    // what goes here?
    Sem_post(&s2);
    Sem_wait(&s1);
    printf("child 2: after\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p1, p2;
    if (argc > 1) sleeper = atoi(argv[1]);
    printf("parent: begin\n");
    // init semaphores here
    Sem_init(&s1, 0);
    Sem_init(&s2, 0);
    Pthread_create(&p1, NULL, child_1, NULL);
    Pthread_create(&p2, NULL, child_2, NULL);
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);
    printf("parent: end\n");
    return 0;
}
