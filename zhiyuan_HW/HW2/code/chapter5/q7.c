// Q7: the child closes standard output, then calls printf().
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("parent: before fork\n");
    fflush(stdout);
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        close(STDOUT_FILENO);
        int n = printf("child: can you see me?\n");
        int f = fflush(stdout);
        // stderr (fd 2) is still open, so report what happened there
        fprintf(stderr, "child (stderr): printf returned %d, fflush returned %d (%s)\n",
                n, f, strerror(errno));
    } else {
        wait(NULL);
        printf("parent: stdout still works\n");
    }
    return 0;
}
