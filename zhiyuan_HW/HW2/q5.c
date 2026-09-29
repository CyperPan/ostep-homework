// Q5: what wait() returns in the parent, and in a child with no children.
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        int wc = wait(NULL);
        printf("child  (pid %d): wait() returned %d, errno = %s\n",
               (int) getpid(), wc, strerror(errno));
        exit(42);
    } else {
        int status;
        int wc = wait(&status);
        printf("parent (pid %d): fork() returned %d, wait() returned %d\n",
               (int) getpid(), rc, wc);
        if (WIFEXITED(status))
            printf("parent: child exit status = %d\n", WEXITSTATUS(status));
    }
    return 0;
}
