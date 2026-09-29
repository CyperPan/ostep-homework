// Q3: make the child print first without calling wait() in the parent.
// The parent blocks on read() from a pipe until the child writes to it.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int p[2];
    if (pipe(p) < 0) {
        perror("pipe");
        exit(1);
    }
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        close(p[0]);
        printf("hello\n");
        fflush(stdout);
        write(p[1], "x", 1); // signal the parent
        close(p[1]);
    } else {
        close(p[1]);
        char c;
        read(p[0], &c, 1);   // blocks until the child has printed
        close(p[0]);
        printf("goodbye\n");
    }
    return 0;
}
