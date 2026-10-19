// Q6: read an element of an array after freeing it.
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int *data = malloc(100 * sizeof(int));
    for (int i = 0; i < 100; i++)
        data[i] = i;
    free(data);
    printf("data[50] after free = %d\n", data[50]);
    return 0;
}
