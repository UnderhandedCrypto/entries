

           Neville Longbottom's 2017 Underhanded Crypto Contest Entry


                               -- Introduction --

My submission to the Underhanded Crypto Contest (2017) is a simple tool for
encrypting messages with AES-128-CBC. You can compare the tool's output with
output from OpenSSL's implementation of AES-128-CBC and they will agree exactly
on short messages. However, on longer messages, a bug is triggered causing the
encryption key to be zeroed out with high probability. Hence long ciphertexts
can be all decrypted with the key "00000000000000000000000000000000". The same
bug is triggered upon decryption so that the incorrectly-encrypted ciphertexts
get decrypted to the correct plaintext, making the backdoor harder to detect.
Due to the nature of the bug and because the hex-encoded ciphertexts are twice
as long as their plaintexts, there is a small range of ciphertext lengths that
will be decrypted incorrectly (making the backdoor detectable).

The files are organized as follows.

    submission
    │
    ├── easycrypt.c           o The encryption/decryption tool's source code.
    ├── Makefile              o A makefile for building the code. Run `make`.
    │
    ├── test                  o Unit tests for the encryption which compare
    │   ├── small-message.txt   it against OpenSSL to make sure we've
    │   └── test.sh             implemented AES-128-CBC "correctly".
    │
    ├── test-backdoor         o Tests that reveal the backdoor, demonstrating
    │   └── test-backdoor.sh    that long messages can be decrypted with the
    │                           key "00000000000000000000000000000000".
    │
    └── tinyaes               o A small AES library I copied off of GitHub.
        ├── aes.c               https://github.com/kokke/tiny-AES128-C
        └── aes.h



                               -- The Backdoor --

The backdoor depends heavily on how the compiler decides to lay out variables
and buffers on the stack. I have only tested it on the following systems, so you
may wish to acquire a similar one if you're going to try to reproduce it.

    1. Arch Linux with gcc 6.3.1.
    2. Debian 8.7 with gcc 4.9.2 (Debian 4.9.2-10).

The heart of this backdoor is the following line in bin2hex():

    memset(hex_out, 0, strlen(hex_out));

If we pass uninitialized memory to the hex_out parameter of bin2hex(), it will
zero-out the memory up until the next zero byte. This is how the backdoor will
zero-out the key.

At the top of encrypt(), the following variables are defined.

    unsigned char key[16];
    char buffer[SM_BUF_SIZE];
    unsigned char cipher_block[SM_BUF_SIZE];
    unsigned char IV[16];
    size_t i = 0;

These variables have been arranged specially so that the compiler places
`buffer` just before `key` on the stack. When the backdoor is being triggered,
encrypt()...

    1. Decodes the key and IV from hex into byte arrays. If we allowed main() to
    pass in a pointer to the raw binary key on its stack frame then to zero it
    out we would have to zero through the saved-RIP and the program would crash
    when encrypt() returns. Accepting the key in hex gives us an excuse to have
    our own `key` buffer and control where it is on the stack.

    2. Fills `buffer` with the hex-encoded ciphertext header. We're passing the
    uninitialized `buffer` into getHexHeader() which seems fine since
    getHexHeader() is only supposed to write the header string into the buffer
    we pass it. However, getHexHeader() internally passes the uninitialized
    `buffer` to bin2hex() so the memset() line will start zeroing out at the
    start of `buffer` and stop at the next zero byte.

    3. Encrypts the plaintext with AES-128 in CBC mode, using `key` as the key.
    Since `key` has been zeroed out, the plaintext is encrypted with a zero key.

The backdoor works exactly the same way inside decrypt(). Any other bugs that
exist in this code are accidental and not part of the submission.

In order for the zeroing-out to work properly, the uninitialized `buffer` memory
needs to contain only nonzero bytes. Otherwise, the zeroing-out won't reach the
key. This is guaranteed for long plaintexts/ciphertexts by the readString()
function which main calls directly before encrypt() or decrypt(). When the input
is long enough, readString()'s larger `buffer` will be completely filled with
nonzero bytes and so by the time encrypt() or decrypt() are called, their
smaller `buffer` (residing at roughly the same location on the stack) will be
full of nonzero bytes.

The `buffer` in readString() is initialized to all zeroes, so if the input is
too short then there's a good chance of a zero byte remaining between the start
of encrypt()'s or decrypt()'s `buffer` and the `key` buffer, preventing the key
from being zeroed-out.

Note that if the key itself contains a zero byte then the zeroing-out will stop
there. For a random 16-byte key, the probability that the entire key gets zeroed
out is approximately 1 - 16/256 = 93.8%. Even in cases where the key is not
completely zeroed-out it will still be greatly weakened with high probability.



                                  -- Tests --

The test/test.sh script compares the tool's output to OpenSSL's for a small
plaintext. Hopefully this will be enough to convince some people that the
implementation is correct. If not, it could even be "improved" to check some of
the test vectors that are published for AES in CBC mode, as long as they are
shorter than the threshold for activating the backdoor.

The backdoor can be tested by running test-backdoor/test-backdoor.sh. I obtain
the following output showing that the backdoor is inactive up to 61-block
(976-byte) plaintexts, decryption is broken for 62-to-124-block (992-byte to
1984-byte) plaintexts, and the backdoor is active for 125-block (2496-byte) and
longer plaintexts.

                        0 BLOCKS:    CORRECT   INACTIVE
                        1 BLOCKS:    CORRECT   INACTIVE
                        2 BLOCKS:    CORRECT   INACTIVE
                        3 BLOCKS:    CORRECT   INACTIVE
                        4 BLOCKS:    CORRECT   INACTIVE
                                    ...     ...
                       58 BLOCKS:    CORRECT   INACTIVE
                       59 BLOCKS:    CORRECT   INACTIVE
                       60 BLOCKS:    CORRECT   INACTIVE
                       61 BLOCKS:    CORRECT   INACTIVE
                       62 BLOCKS:  INCORRECT   INACTIVE
                       63 BLOCKS:  INCORRECT   INACTIVE
                       64 BLOCKS:  INCORRECT   INACTIVE
                       65 BLOCKS:  INCORRECT   INACTIVE
                                    ...     ...
                      121 BLOCKS:  INCORRECT   INACTIVE
                      122 BLOCKS:  INCORRECT   INACTIVE
                      123 BLOCKS:  INCORRECT   INACTIVE
                      124 BLOCKS:  INCORRECT   INACTIVE
                      125 BLOCKS:    CORRECT     ACTIVE
                      126 BLOCKS:    CORRECT     ACTIVE
                      127 BLOCKS:    CORRECT     ACTIVE
                      128 BLOCKS:    CORRECT     ACTIVE
                                    ...     ...



                               -- Improvements --

There are several ways to improve upon this backdoor which I did not have time
to try. Someone else may take these up as challenges.

    1. Adjust the buffer sizes and stack layout in encrypt() and decrypt() to
    minimize (or eliminate!) the range of lengths decryption is broken for.

    2. Figure out how to precisely tune the length at which the backdoor
    activates by adjusting the buffer sizes.

    3. An auditor would likely recognize the memset() line in bin2hex() as out
    of place. There is probably a better excuse to run the memset() line --
    maybe rearrange the code and zero-out the hex key "for security" and somehow
    it happens to be missing its null-termination byte.



                                  -- Thanks --

Thank you greatly for the time it takes to judge my entry as well as the others.

        -- Neville Longbottom
