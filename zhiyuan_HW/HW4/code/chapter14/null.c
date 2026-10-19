// Q1-Q3: dereference a NULL pointer.
#include <stdio.h>

int main(int argc, char *argv[]) {
    int *p = NULL;
    printf("about to dereference a NULL pointer\n");
    fflush(stdout);
    printf("value: %d\n", *p);
    return 0;
}
