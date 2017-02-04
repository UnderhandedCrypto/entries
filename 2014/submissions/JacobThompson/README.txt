Submission to Underhanded Cryptography Contest
Jacob Thompson <jakethompson1@gmail.com>
Dec. 1, 2014

Easy Encryption Program for Linux

This submission includes a program to allow a user to easily perform
"secure" password-based encryption of disk files.

The program uses OpenSSL, and is compiled as follows:

 gcc -o easycrypt easycrypt.c -lcrypto

The -DDEBUG flag can also be passed to compile a program that runs in
debug mode.

Cryptographic Design
====================

Files are encrypted using 128-bit AES in CBC mode.  Both the key and IV are
securely generated using the Linux /dev/random device.

The randomly generated "file key" is encrypted using another key
generated from the user's password and a random 256-byte salt based on
an algorithm inspired by PBKDF1:

	buf = password || salt
	for i = 1 to 999
	    buf = MD5(buf)

When encrypting the salt, encrypted file key and the IV are prepended
to the AES-128-CBC ciphertext.

The Bug
=======

Depending on the available entropy, the Linux /dev/random device may
return a "short read," that is, we may request 16 bytes but receive fewer.

In case this occurs, the program is written to generate the key and IV
in chunks while providing user feedback as to the percentage
generated, so that the user can move the mouse or type on the keyboard
to generate more entropy.

In the "keygen" function, the have_iv variable keeps track of whether
we are currently generating the IV (have_iv==0), or key (have_iv==1).
The "i" variable tracks the number of bytes of the IV or key that have
already been generated.  The following line reads from /dev/random
into the correct variable, offset by the number of bytes already received:

     err = read (fd, have_iv ? key : iv + i, 16 - i);
                                        ^
                                        oops

The bug is that the precendence of the ?: operator is often not
visually obvious.  (have_iv ? key : iv + i) means that if we are
generating the key, we always write a partial key starting from the
beginning of the array, while IVs are always generated correctly.  The
code should be:

     err = read (fd, (have_iv ? key : iv) + i, 16 - i);	

Compounding the issue, the problem occurs only intermittently, once
the entropy pool is drained; if the /dev/random device returns 16
bytes at once, the bug is irrelevant.  To cause the short read to
happen, run the program several times in succession, or read from
/dev/random to drain the pool.  When the program is compiled with
-DDEBUG the key generation problem is obvious:

  $ dd if=/dev/random of=/dev/null bs=1
  ^C424+0 records in
  424+0 records out
  424 bytes (424 B) copied, 2.90167 s, 0.1 kB/s

  $ ./easycrypt -e infile outfile.bin
  Enter password to encrypt infile: Please move your mouse to generate entropy
  100.0% Generating IV... /
  100.0% Generating key... \...done!
  (DEBUG) password-derived key, kkey: d7950fde3157fc3b8dad4491d1e5efb3
  (DEBUG) encrypted file key, ekey: 2101fc79aeb4f59d8da4b2fbc34c03ea
  (DEBUG) file iv: 63cbda0db2bd42d40d603db292559315
  (DEBUG) file key: f0c6036a7a91985b0000000000000000

In this case, two reads from the /dev/random device returned eight
bytes each, but the second read simply overwrote the first due to the
missing "+ i".

Thus despite the 128-bit key length, the key is actually only 64-bits
strong.
