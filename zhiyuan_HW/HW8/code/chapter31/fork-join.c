#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "common_threads.h"

sem_t s; 

void *child(void *arg) {
    sleep(1);  // the child is slow: the parent must still wait for it
    printf("child\n");
    // use semaphore here
    Sem_post(&s);  // tell the parent we are done
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p;
    printf("parent: begin\n");
    // init semaphore here
    Sem_init(&s, 0);  // 0: nothing has happened yet, so a wait must block
    Pthread_create(&p, NULL, child, NULL);
    // use semaphore here
#ifndef BROKEN        // -DBROKEN leaves out the wait, to see what goes wrong
    Sem_wait(&s);     // block until the child has posted
#endif
    printf("parent: end\n");
    return 0;
}
