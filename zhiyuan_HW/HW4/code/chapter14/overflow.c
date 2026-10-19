// Q5: write one element past the end of a malloc'd array.
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int *data = malloc(100 * sizeof(int));
    data[100] = 0;                 // valid indices are 0..99
    printf("wrote data[100] = %d\n", data[100]);
    free(data);
    return 0;
}
