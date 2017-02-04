#include "tinyaesctr.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/*
   Test Vector #1: Encrypting 16 octets using AES-CTR with 128-bit key
   AES Key          : AE 68 52 F8 12 10 67 CC 4B F7 A5 76 55 77 F3 9E
   AES-CTR IV       : 00 00 00 00 00 00 00 00
   Nonce            : 00 00 00 30
   Plaintext String : 'Single block msg'
   Plaintext        : 53 69 6E 67 6C 65 20 62 6C 6F 63 6B 20 6D 73 67
   Counter Block (1): 00 00 00 30 00 00 00 00 00 00 00 00 00 00 00 01
   Key Stream    (1): B7 60 33 28 DB C2 93 1B 41 0E 16 C8 06 7E 62 DF
   Ciphertext       : E4 09 5D 4F B7 A7 B3 79 2D 61 75 A3 26 13 11 B8

   Test Vector #2: Encrypting 32 octets using AES-CTR with 128-bit key
   AES Key          : 7E 24 06 78 17 FA E0 D7 43 D6 CE 1F 32 53 91 63
   AES-CTR IV       : C0 54 3B 59 DA 48 D9 0B
   Nonce            : 00 6C B6 DB
   Plaintext        : 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
                    : 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F
   Counter Block (1): 00 6C B6 DB C0 54 3B 59 DA 48 D9 0B 00 00 00 01
   Key Stream    (1): 51 05 A3 05 12 8F 74 DE 71 04 4B E5 82 D7 DD 87
   Counter Block (2): 00 6C B6 DB C0 54 3B 59 DA 48 D9 0B 00 00 00 02
   Key Stream    (2): FB 3F 0C EF 52 CF 41 DF E4 FF 2A C4 8D 5C A0 37
   Ciphertext       : 51 04 A1 06 16 8A 72 D9 79 0D 41 EE 8E DA D3 88
                    : EB 2E 1E FC 46 DA 57 C8 FC E6 30 DF 91 41 BE 28

   Test Vector #3: Encrypting 36 octets using AES-CTR with 128-bit key
   AES Key          : 76 91 BE 03 5E 50 20 A8 AC 6E 61 85 29 F9 A0 DC
   AES-CTR IV       : 27 77 7F 3F  4A 17 86 F0
   Nonce            : 00 E0 01 7B
   Plaintext        : 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
                    : 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F
                    : 20 21 22 23
   Counter Block (1): 00 E0 01 7B 27 77 7F 3F 4A 17 86 F0 00 00 00 01
   Key Stream    (1): C1 CE 4A AB 9B 2A FB DE C7 4F 58 E2 E3 D6 7C D8
   Counter Block (2): 00 E0 01 7B 27 77 7F 3F 4A 17 86 F0 00 00 00 02
   Key Stream    (2): 55 51 B6 38 CA 78 6E 21 CD 83 46 F1 B2 EE 0E 4C
   Counter Block (3): 00 E0 01 7B 27 77 7F 3F 4A 17 86 F0 00 00 00 03
   Key Stream    (3): 05 93 25 0C 17 55 36 00 A6 3D FE CF 56 23 87 E9
   Ciphertext       : C1 CF 48 A8 9F 2F FD D9 CF 46 52 E9 EF DB 72 D7
                    : 45 40 A4 2B DE 6D 78 36 D5 9A 5C EA AE F3 10 53
                    : 25 B2 07 2F
*/

struct testcase {
        int nblocks;
        uint8 key[KLEN];
        uint8 iv[KLEN];
        uint8 blocks[3][KLEN];
};

struct testcase case1 = {
        .nblocks = 1,
        .key = { 0xAE, 0x68, 0x52, 0xF8, 0x12, 0x10, 0x67, 0xCC, 0x4B, 0xF7, 0xA5, 0x76, 0x55, 0x77, 0xF3, 0x9E },
        .iv = { 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
        .blocks = { { 0xB7, 0x60, 0x33, 0x28, 0xDB, 0xC2, 0x93, 0x1B, 0x41, 0x0E, 0x16, 0xC8, 0x06, 0x7E, 0x62, 0xDF } }
};
struct testcase case2 = {
        .nblocks = 2,
        .key = {  0x7E, 0x24, 0x06, 0x78, 0x17, 0xFA, 0xE0, 0xD7, 0x43, 0xD6, 0xCE, 0x1F, 0x32, 0x53, 0x91, 0x63, },
        .iv = { 0x00, 0x6C, 0xB6, 0xDB, 0xC0, 0x54, 0x3B, 0x59, 0xDA, 0x48, 0xD9, 0x0B, },
        .blocks = {
                { 0x51, 0x05, 0xA3, 0x05, 0x12, 0x8F, 0x74, 0xDE, 0x71, 0x04, 0x4B, 0xE5, 0x82, 0xD7, 0xDD, 0x87, },
                { 0xFB, 0x3F, 0x0C, 0xEF, 0x52, 0xCF, 0x41, 0xDF, 0xE4, 0xFF, 0x2A, 0xC4, 0x8D, 0x5C, 0xA0, 0x37, },
        }
};
struct testcase case3 = {
        .nblocks = 3,
        .key =
        {  0x76, 0x91, 0xBE, 0x03, 0x5E, 0x50, 0x20, 0xA8, 0xAC, 0x6E, 0x61, 0x85, 0x29, 0xF9, 0xA0, 0xDC,},
        .iv =
        {  0x00, 0xE0, 0x01, 0x7B, 0x27, 0x77, 0x7F, 0x3F,  0x4A, 0x17, 0x86, 0xF0,},
        .blocks = {
                {  0xC1, 0xCE, 0x4A, 0xAB, 0x9B, 0x2A, 0xFB, 0xDE, 0xC7, 0x4F, 0x58, 0xE2, 0xE3, 0xD6, 0x7C, 0xD8,},
                {  0x55, 0x51, 0xB6, 0x38, 0xCA, 0x78, 0x6E, 0x21, 0xCD, 0x83, 0x46, 0xF1, 0xB2, 0xEE, 0x0E, 0x4C,},
                {  0x05, 0x93, 0x25, 0x0C, 0x17, 0x55, 0x36, 0x00, 0xA6, 0x3D, 0xFE, 0xCF, 0x56, 0x23, 0x87, 0xE9,},
        }
};

void run_case(struct testcase *c, char *str)
{
        printf("Running %s\n", str);
        memcpy(buf, c->key, KLEN);
        aes_ctr_crypt(NULL, 0, MODE_SETKEY);
        memcpy(buf, c->iv, KLEN);
        aes_ctr_crypt(NULL, 0, MODE_RESETIV);
        for (int i = 0; i < c->nblocks; i++) {
                uint8 zeros[KLEN] = { 0 };
                aes_ctr_crypt(zeros, KLEN, MODE_ENCRYPT | MODE_KEEPKEY);
                for (int j = 0; j < KLEN; j++)
                        assert(zeros[j] == c->blocks[i][j]);
        }
}

int main(int argc, char *argv[])
{
        printf("Running RFC test cases\n");
        run_case(&case1, "case 1");
        run_case(&case2, "case 2");
        run_case(&case3, "case 3");
}

void fill_random(void)
{
        assert(!"Should not be called for explicit IV");
}
