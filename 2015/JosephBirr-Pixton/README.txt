NAME OF PROJECT: RFC6979 is not optional
WAS THIS A TEAM PROJECT: no
PROJECT LICENSE: CC0

CHALLENGE (GPG KEY LEAK / PASSWORD HASHING BACKDOOR): GPG key leak

DESCRIPTION:
See submission/patch.txt for the underhandedness.  This is for GnuPG 1.4.

DSA needs a entirely secret, unique-per-key number (`k') for each signature.
If you fail at these constraints (like it being low entropy, or one bit being
biased, or reusing a k value) your private key is recoverable from signatures.

k needs to be chosen uniformly in [0,q). gnupg's choice of k starts[1] by
choosing |q| random bits, setting the top bit and seeing if the result is <q.
If it's not, it chooses the top 32 bits again.  The probability of this happening
is very low (will vary by group, a group I generated gave a 4.5% hit rate).

Because k is a super-sensitive security parameter, it obviously needs to
be zeroised instead of left on the heap!  Unfortunately my patch clears the
buffer when the candidate is >=q, as well as at the end.  That means some
signatures have a 32-bit entropy k.  This is enough to brute force.

To demonstrate that, submission/recover.py unpicks the resulting pgp signature
to extract everything needed to recover k from a sample signature in
submission/sigs/attack.asc.  There's also submission/recoverk.c which uses
openssl's bignum library to search faster.

Instant gratification:

  $ python recover.py > script.sh
  $ make recoverk
  $ sh script.sh
  k search: 00000000..0000ffff
  search 00000000
  search 00001000
  <about an hour passes>
  k search: 003dffc2..003effc1
  search 003e0000
  search 003e1000
  search 003e2000
  search 003e3000
  with k = 003e3dfb, we found x = 3B29FC0B63E77D73C4956FD35F6A5E3E0DCA04D55A3A3197BB82C6CBC32985A5

recoverk can search 16-bits worth of space in 50 CPU seconds.  The search is
embarrassingly parallel, so you can hire an EC2 m4.10xlarge (40 cores)
and extract the private key from an affected signature for about USD50 in 22
hours.

The user will not see any difference: their signatures will always still work, and
be properly randomised with overwhelming probability.  I think it's also likely to
be overlooked in source code review.

[1]: see cipher/dsa.c gen_k(). note that GnuPG 2.0 is different and unaffected.

Files in submission:
- dsa.orig.c: unchanged cipher/dsa.c.  GPL license.
- dsa.patched.c: altered cipher/dsa.c.  GPL license.
- dsa.py: comedy insecure DSA in python.
- generate-sigs.sh: generate a shedload of signatures and keep the ones affected.
- gnupg/: a test key recovered by submission, passphrase is 'test'.
- Makefile: builds recoverk.
- patch.txt: the patch.
- recoverk.c: DSA k recovery program.  Needs libcrypto.
- recover.py: unpicks sigs/attack.asc and outputs shell invocations of recoverk to
  do the work.
- sigs/: directory of affected signatures.
