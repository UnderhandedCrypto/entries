#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <openssl/bn.h>

static uint32_t unhexc(char v)
{
  if (v >= '0' && v <= '9')
    return v - '0';
  else if (v >= 'a' && v <= 'f')
    return v - 'a' + 10;
  else if (v >= 'A' && v <= 'F')
    return v - 'A' + 10;
  return 0;
}

static uint32_t unhex16(const char *v)
{
  return (unhexc(v[0]) << 4) | unhexc(v[1]);
}

static uint32_t unhex(const char *v)
{
  assert(strlen(v) == 8);
  uint32_t r = 0;
  r |= unhex16(v + 0) << 24;
  r |= unhex16(v + 2) << 16;
  r |= unhex16(v + 4) << 8;
  r |= unhex16(v + 6);
  return r;
}

int main(int argc, char **argv)
{
  /* 9 arguments (all in hex):
   *   startk
   *   stopk    -- bounds for k search
   *   p
   *   q
   *   g        -- group
   *   y        -- public key
   *   r
   *   s        -- signature
   *   h        -- hash of message */
  assert(argc == 10);
  uint32_t startk = unhex(argv[1]),
           stopk = unhex(argv[2]);

  BIGNUM *p = NULL, *q = NULL, *g = NULL,
         *y = NULL, *sigr = NULL, *sigs = NULL, *h = NULL;
  assert(BN_hex2bn(&p, argv[3]));
  assert(BN_hex2bn(&q, argv[4]));
  assert(BN_hex2bn(&g, argv[5]));
  assert(BN_hex2bn(&y, argv[6]));
  assert(BN_hex2bn(&sigr, argv[7]));
  assert(BN_hex2bn(&sigs, argv[8]));
  assert(BN_hex2bn(&h, argv[9]));

  printf("k search: %08x..%08x\n", startk, stopk);

  BIGNUM *candidatek = BN_new();

  BN_CTX *ctx = BN_CTX_new();
  BIGNUM *rinv = BN_mod_inverse(NULL, sigr, q, ctx);
  assert(rinv);

  BIGNUM *tmp1 = BN_new(),
         *tmp2 = BN_new();

  for (uint32_t k = startk; k < stopk; k++)
  {
    if ((k & 0xfff) == 0) printf("search %08x\n", k);
    BN_set_word(candidatek, k);
    BN_lshift(candidatek, candidatek, 224); // nb entropy is in top bits

    // x = ((s * k - h) * rinv) % group.q
    assert(BN_mod_mul(tmp1, sigs, candidatek, q, ctx));
    assert(BN_sub(tmp1, tmp1, h));
    assert(BN_mod_mul(tmp2, tmp1, rinv, q, ctx));

    // check candidate key
    assert(BN_mod_exp(tmp1, g, tmp2, p, ctx));
    if (BN_cmp(tmp1, y) == 0)
    {
      char *ss = BN_bn2hex(tmp2);
      printf("with k = %08x, we found x = %s\n", k, ss);
      return 1; // so we can set -e in calling script to stop search!
    }
  }

  printf("not found\n");

  return 0;
}
