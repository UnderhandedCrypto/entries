void swap(int *x, int *y)
{
    *x ^= *y;
    *y ^= *x;
    *x ^= *y;
}

void rc4_crypt(char *data, int datasize, char *key, int keysize)
{
    int s[256];
    for (int i = 0; i < 256; i++) {
        s[i] = i;
    }

    int j = 0;
    for (int i = 0; i < 256; i++) {
        j = (j + s[i] + (int)(key[i % keysize])) % 256;
        swap(&s[i], &s[j]);
    }

    int i = 0;
    j = 0;
    for (int c = 0; c < datasize; c++) {
        i = (i + 1) % 256;
        j = (j + s[i]) % 256;
        swap(&s[i], &s[j]);
        data[c] = data[c] ^ s[(s[i] + s[j]) % 256];
    }
}

/* example starts here */
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* the size of the data to be encrypted */
#define DATA_SIZE (1024 * 100)

int main()
{
    /* initialize key, not really a key, but who cares */
    char key[] = {0};
    int keysize = 1;

    /* initialize data */
    char *data = malloc(DATA_SIZE);
    memset(data, 'x', DATA_SIZE);

    rc4_crypt(data, strlen(data), key, keysize);
    /* we use write because printf stops printing after the first NULL byte */
    write(1, data, DATA_SIZE);
    printf("\n");
    return 0;
}
