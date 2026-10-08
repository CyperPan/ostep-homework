// Q1: a variable set before fork() is copied into the child.
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int x = 100;
    printf("before fork: x = %d (pid %d)\n", x, (int) getpid());
    fflush(stdout); // otherwise the buffered line may be copied into the child
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("child:  x = %d (inherited)\n", x);
        x = 200;
        printf("child:  set x = %d, &x = %p\n", x, (void *) &x);
    } else {
        x = 300;
        printf("parent: set x = %d, &x = %p\n", x, (void *) &x);
        wait(NULL);
        printf("parent: after child exits, x = %d\n", x);
    }
    return 0;
}
