// Q8: two children connected by a pipe, like the shell command "ls -1 / | wc -l".
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int p[2];
    if (pipe(p) < 0) {
        perror("pipe");
        exit(1);
    }

    int writer = fork();
    if (writer == 0) {
        dup2(p[1], STDOUT_FILENO); // stdout -> write end of pipe
        close(p[0]);
        close(p[1]);
        execlp("ls", "ls", "-1", "/", NULL);
        perror("execlp ls");
        exit(1);
    }

    int reader = fork();
    if (reader == 0) {
        dup2(p[0], STDIN_FILENO);  // stdin <- read end of pipe
        close(p[0]);
        close(p[1]);
        execlp("wc", "wc", "-l", NULL);
        perror("execlp wc");
        exit(1);
    }

    // the parent must close both ends, or wc never sees end-of-file
    close(p[0]);
    close(p[1]);
    waitpid(writer, NULL, 0);
    waitpid(reader, NULL, 0);
    return 0;
}
