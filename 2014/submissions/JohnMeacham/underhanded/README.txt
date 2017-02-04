Underhanded Crypto Contest entry by John Meacham (john@repetae.net)
http://notanumber.net/

Spoilers as to the security flaw and demonstrations of the intended attack are
in the exploit/ directory. Everything else is spoiler-free.

This entry contains code derived from the public domain tiny-aes128-c project.
--

tinyaesctr - A portable, minimal rfc3686 compliant implementation of AES encryption in CTR mode. 

This implementation is specifically designed for resource constrained devices,
It makes use of static memory buffers and minimal use of pointers to better fit
the CPUs used in embedded systems.  

files:

        - Makefile
          build file for linux systems
        - tinyaesctr.h
          The interface to the AES algorithm, API is described in comments of this file.
        - tinyaesctr.c
          The implementation of AES128 in CTR mode.
        - aesctr.c
          A program demonstrating use of the API to encrypt and decrypt on the command line. 
          This can also be used to verify the algorithm against OpenSSLs aes-ctr implementation.
        - aesctrtest.c
          Test cases as defined by RFC3686
        - aes_tables.c
          Static tables used by the AES algorithm. Should be placed in ROM.
        - README.txt
          this file


testing:

running ./aesctrtest will run the RFC test vectors and assert they are correct.

the ./aesctr command line program can be used to encrypt (or decrypt) standard
input to standard error. It takes two arguments '-k' to specify the key and
'-v' to specify the iv. If '-k' is 'random' a random key is chosen and printed
to stder, if '-v' is missing a random IV is chosen and printed to stderr.

testing against openssl can be done via the aesctr program by doing

./aesctr -k keydata -v ivdata  | openssl enc -d -aes-128-ctr -K keydata -v ivdata 

I encourage testing against various files, keys, and ivs.
