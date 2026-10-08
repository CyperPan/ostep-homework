// Q6: waitpid() lets the parent wait for one specific child.
// The first child is slow and the second is fast, but we reap the slow one first.
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int slow = fork();
    if (slow == 0) {
        sleep(2);
        printf("slow child (pid %d) exiting\n", (int) getpid());
        exit(1);
    }
    int fast = fork();
    if (fast == 0) {
        printf("fast child (pid %d) exiting\n", (int) getpid());
        exit(2);
    }

    int status;
    // WNOHANG: poll without blocking
    int wc = waitpid(slow, &status, WNOHANG);
    printf("parent: waitpid(slow, WNOHANG) returned %d (still running)\n", wc);

    wc = waitpid(slow, &status, 0);
    printf("parent: waitpid(slow) returned %d, exit status %d\n", wc, WEXITSTATUS(status));
    wc = waitpid(fast, &status, 0);
    printf("parent: waitpid(fast) returned %d, exit status %d\n", wc, WEXITSTATUS(status));
    return 0;
}
