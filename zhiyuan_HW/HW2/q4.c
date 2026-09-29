// Q4: run /bin/ls with every exec() variant.
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define NVARIANTS 6

static const char *names[NVARIANTS] = {
    "execl", "execle", "execlp", "execv", "execvp", "execvpe"
};

int main(int argc, char *argv[]) {
    char *args[] = {"ls", "-ld", "/tmp", NULL};
    char *envp[] = {"MYVAR=hello", NULL};

    for (int i = 0; i < NVARIANTS; i++) {
        printf("%-8s: ", names[i]);
        fflush(stdout);
        int rc = fork();
        if (rc < 0) {
            fprintf(stderr, "fork failed\n");
            exit(1);
        } else if (rc == 0) {
            switch (i) {
            case 0: execl("/bin/ls", "ls", "-ld", "/tmp", NULL); break;
            case 1: execle("/bin/ls", "ls", "-ld", "/tmp", NULL, envp); break;
            case 2: execlp("ls", "ls", "-ld", "/tmp", NULL); break;
            case 3: execv("/bin/ls", args); break;
            case 4: execvp("ls", args); break;
            case 5: execvpe("ls", args, envp); break;
            }
            perror(names[i]); // only reached if exec failed
            exit(1);
        }
        wait(NULL);
    }
    return 0;
}
