// Q7: pass a pointer into the middle of an allocated array to free().
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int *data = malloc(100 * sizeof(int));
    printf("calling free(data + 50)\n");
    fflush(stdout);
    free(data + 50);
    printf("free() returned\n");
    return 0;
}
