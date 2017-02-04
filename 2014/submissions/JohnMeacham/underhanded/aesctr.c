#include "tinyaesctr.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

static boolean hexchar(int *c);
static boolean read_hex(uint8 buf[KLEN], char *s, char *emsg);

char *optk, *optv;
boolean do_decrypt, embed_iv;

int main(int argc, char *argv[])
{
        int opt;
        while ((opt = getopt(argc, argv, "k:v:")) != -1)
                switch (opt) {
                case 'k':
                        optk = optarg;
                        break;
                case 'v':
                        optv = optarg;
                        break;
                default:
                        fprintf(stderr, "Usage %s -k key [-v iv]\n", argv[0]);
                        exit(EXIT_FAILURE);
                }
        if (!optk) {
                fprintf(stderr, "key should be specified with -k\n");
                fprintf(stderr, "Usage %s -k key [-v iv]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
        if (!strcasecmp("random", optk)) {
                fill_random();
                fprintf(stderr, "Key: ");
                for (int i = 0; i < KLEN; i++)
                        fprintf(stderr, "%02x", buf[i]);
                fprintf(stderr, "\n");
        } else
                read_hex(buf, optk, "key");
        aes_ctr_crypt(NULL, 0, MODE_SETKEY);
        if (optv && strcasecmp(optv, "random")) {
                read_hex(buf, optv, "iv");
                aes_ctr_crypt(NULL, 0, MODE_SETIV);
        } else {
                aes_ctr_crypt(NULL, 0, MODE_GENIV);
                fprintf(stderr, "IV: ");
                for (int i = 0; i < KLEN; i++)
                        fprintf(stderr, "%02x", buf[i]);
                fprintf(stderr, "\n");
        }
        uint8 buffer[1024];
        while (!feof(stdin)) {
                int n = fread(buffer, 1, sizeof(buffer), stdin);
                aes_ctr_crypt(buffer, n, MODE_ENCRYPT|MODE_KEEPKEY);
                fwrite(buffer, 1, n, stdout);
        }
        return 0;
}

boolean hexchar(int *c)
{
        if (*c >= '0' && *c <= '9')
                *c -= '0';
        else if (*c >= 'a' && *c <= 'f')
                *c -= 'a' - 10;
        else if (*c >= 'A' && *c <= 'F')
                *c -= 'A' - 10;
        else
                return FALSE;
        return TRUE;
}

boolean read_hex(uint8 buf[KLEN], char *s, char *emsg)
{
        if (strlen(s) != 2 * KLEN) {
                if (emsg) {
                        fprintf(stderr, "%s must be %i hex characters\n", emsg, 2 * KLEN);
                        exit(EXIT_FAILURE);
                } else
                        return FALSE;
        }
        for (int i = 0; i < KLEN; i++) {
                int c1 = s[i * 2];
                int c2 = s[i * 2 + 1];
                if (!hexchar(&c1) || !hexchar(&c2)) {
                        if (emsg) {
                                fprintf(stderr, "%s must consist of hex characters [0-9a-f]", emsg);
                                exit(EXIT_FAILURE);
                        } else
                                return FALSE;
                }
                buf[i] = c1 << 4 | c2;
        }
        return TRUE;
}

/* Place KLEN cryptographically random bytes in buf.
 *
 * This must be replaced with a cryptographically strong random number
 * source on a system.
 */
void fill_random(void)
{
        FILE *random;
        int ret;
        random = fopen("/dev/urandom", "rb");
        if (!random)
                err(1, "/dev/urandom");
        ret = fread(buf, 1, KLEN, random);
        if (ret != KLEN)
                err(1, "fread /dev/urandom");
        fclose(random);
}
