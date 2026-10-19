// Q4: allocate memory and forget to free it.
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int *data = malloc(10 * sizeof(int));
    for (int i = 0; i < 10; i++)
        data[i] = i;
    printf("data[9] = %d; exiting without free()\n", data[9]);
    return 0;
}
