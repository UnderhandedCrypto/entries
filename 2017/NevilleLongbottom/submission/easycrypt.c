// WARNING: This code contains a BACKDOOR.

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

// https://github.com/kokke/tiny-AES128-C
#include "tinyaes/aes.h"

#define HEADER "UnderhandCrypto1"
#define MAX_MESSAGE_LENGTH 65536
#define LG_BUF_SIZE 2048
#define SM_BUF_SIZE 512

void printUsage();
void readString(char *string_out, size_t max_length);
void encrypt(const char *plaintext, char *hex_key, char *hex_IV, char *ciphertext_out);
void decrypt(const char *ciphertext, char *hex_key, char *hex_IV, char *plaintext_out);
void bin2hex(unsigned char *str, size_t len, char *hex_out);
void getHexHeader(char *header_hex_string);
void hex2bin(const char *hex, unsigned char *bin_out);
inline int hexitToValue(char h);

int main(int argc, char **argv)
{
    char input_string[2*MAX_MESSAGE_LENGTH + 1] = {0};
    char output_string[2*MAX_MESSAGE_LENGTH + 1] = {0};

    if (argc != 4) {
        printUsage();
        return 1;
    }

    if (strcmp(argv[1], "encrypt") == 0) {
        readString(input_string, MAX_MESSAGE_LENGTH);
        encrypt(input_string, argv[2], argv[3], output_string);
    } else if (strcmp(argv[1], "decrypt") == 0) {
        readString(input_string, 2*MAX_MESSAGE_LENGTH);
        decrypt(input_string, argv[2], argv[3], output_string);
    } else {
        printUsage();
        return 1;
    }

    printf("%s", output_string);

    return 0;
}

void printUsage()
{
    printf("Usage: ./easycrypt [encrypt|decrypt] 128-bit-key-as-lowercase-hex 128-bit-IV-as-lowercase-hex\n");
}

void readString(char *string_out, size_t max_length)
{
    size_t characters_read = 0;
    size_t retval = 0;
    char buffer[LG_BUF_SIZE] = {0};
    while (characters_read <= max_length) {
        retval = fread(buffer, 1, LG_BUF_SIZE, stdin);
        memcpy(string_out + characters_read, buffer, retval);
        characters_read += retval;
        if (retval < LG_BUF_SIZE) {
            break;
        }
    }
    string_out[characters_read] = '\0';
}

void encrypt(const char *plaintext, char *hex_key, char *hex_IV, char *ciphertext_out)
{
    unsigned char key[16];
    char buffer[SM_BUF_SIZE];
    unsigned char cipher_block[SM_BUF_SIZE];
    unsigned char IV[16];
    size_t i = 0;

    /* Start with an empty ciphertext. */
    ciphertext_out[0] = '\0';

    /* Convert the hex key to bytes. */
    if (strlen(hex_key) != 32) {
        printf("Wrong key size!\n");
        exit(1);
    }
    hex2bin(hex_key, key);

    /* Convert the hex IV to bytes. */
    if (strlen(hex_IV) != 32) {
        printf("Wrong IV size!\n");
        exit(1);
    }
    hex2bin(hex_IV, IV);

    /* Append the version header. */
    getHexHeader(buffer);
    strcat(ciphertext_out, buffer);

    /* Encrypt each block (CBC mode). */
    for (i = 0; i < strlen(plaintext); i += 16) {

        if (i + 16 < strlen(plaintext)) {
            memcpy(buffer, plaintext + i, 16);
        } else {
            /* Zero pad. */
            size_t remaining = strlen(plaintext) - i;
            memcpy(buffer, plaintext + i, remaining);
            memset(buffer + remaining, 0, 16 - remaining);
        }

        /* Apply the IV (or block chaining in later iterations) */
        for (int j = 0; j < 16; j++) {
            buffer[j] ^= IV[j];
        }

        AES128_ECB_encrypt((unsigned char *)buffer, key, cipher_block);
        memcpy(IV, cipher_block, 16);
        bin2hex(cipher_block, 16, buffer);
        strcat(ciphertext_out, buffer);
    }
}

void decrypt(const char *ciphertext, char *hex_key, char *hex_IV, char *plaintext_out)
{
    unsigned char key[16];
    char buffer1[SM_BUF_SIZE];
    unsigned char buffer2[SM_BUF_SIZE];
    unsigned char IV[16];
    size_t i = 0;

    /* Start with an empty plaintext. */
    plaintext_out[0] = '\0';

    /* Convert the hex key to bytes. */
    if (strlen(hex_key) != 32) {
        printf("Wrong key size!\n");
        exit(1);
    }
    hex2bin(hex_key, key);

    /* Convert the hex IV to bytes. */
    if (strlen(hex_IV) != 32) {
        printf("Wrong IV size!\n");
        exit(1);
    }
    hex2bin(hex_IV, IV);

    /* Check the version header. */
    getHexHeader(buffer1);
    if (strlen(ciphertext) <= strlen(buffer1)) {
        printf("Ciphertext is not big enough!\n");
        exit(1);
    }
    if (memcmp(buffer1, ciphertext, strlen(buffer1)) != 0) {
        printf("Ciphertext is not valid; missing header!\n");
        exit(1);
    }

    /* Skip over the header. */
    ciphertext = ciphertext + strlen(buffer1);

    /* Check that the ciphertext is a valid size. */
    if (strlen(ciphertext) % 32 != 0) {
        printf("Invalid ciphertext length!\n");
        exit(1);
    }

    /* Decrypt each block (CBC mode). */
    for (i = 0; i < strlen(ciphertext); i += 32) {
        /* Convert this block from hex to bytes. */
        memcpy(buffer1, ciphertext + i, 32);
        buffer1[32] = '\0';
        hex2bin(buffer1, buffer2);

        AES128_ECB_decrypt(buffer2, key, (unsigned char *)buffer1);

        /* Apply the IV (or block chaining in later iterations) */
        for (int j = 0; j < 16; j++) {
            buffer1[j] ^= IV[j];
        }

        /* Save this ciphertext block to be the IV next time. */
        memcpy(IV, buffer2, 16);

        /* Concatenate this block to the overall string. */
        buffer1[16] = '\0';
        strcat(plaintext_out, buffer1);
    }
}

void getHexHeader(char *header_hex_string)
{
    bin2hex((unsigned char*)HEADER, strlen(HEADER), header_hex_string);
}

void bin2hex(unsigned char *bin, size_t len, char *hex_out)
{
    char tohex[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    memset(hex_out, 0, strlen(hex_out));
    for (size_t i = 0; i < len; i++) {
        hex_out[i*2] = tohex[(bin[i] >> 4) & 0xF];
        hex_out[i*2 + 1] = tohex[bin[i] & 0xF];
    }
    hex_out[len*2] = '\0';
}

void hex2bin(const char *hex, unsigned char *bin_out)
{
    if (strlen(hex) % 2 != 0) {
        printf("Odd length; input is not a hex string!\n");
        exit(1);
    }

    for (size_t i = 0; i < strlen(hex)/2; i++) {
        bin_out[i] = (hexitToValue(hex[i*2]) << 4) + hexitToValue(hex[i*2+1]);
    }
}

int hexitToValue(char h)
{
    if ('a' <= h && h <= 'f') {
        return (h - 'a') + 10;
    } else if ('0' <= h && h <= '9') {
        return (h - '0');
    } else {
        printf("Invalid character; input is not a hex string!\n");
        exit(1);
    }
}
