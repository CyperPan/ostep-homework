// Q2: parent and child share a file descriptor opened before fork().
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define LINES 5

int main(int argc, char *argv[]) {
    int fd = open("q2.output", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    const char *who = (rc == 0) ? "child " : "parent";
    char buf[64];
    for (int i = 0; i < LINES; i++) {
        snprintf(buf, sizeof(buf), "%s line %d\n", who, i);
        if (write(fd, buf, strlen(buf)) < 0) {
            perror("write");
            exit(1);
        }
    }
    if (rc > 0) {
        wait(NULL);
        printf("file offset after both wrote: %ld\n", (long) lseek(fd, 0, SEEK_CUR));
    }
    close(fd);
    return 0;
}
