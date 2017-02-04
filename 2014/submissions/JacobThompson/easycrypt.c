/*
 * easycrypt.c
 *
 * Copyright (c) 2014 Jacob Thompson, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.  
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Easy password-based symmetrical encryption program.
 * for 2014 Underhanded Crypto Contest.
 *
 */


#include <termios.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <string.h>
#include <stdlib.h>

static struct termios termios;

#define WAIT_SIZE 4
static const char wait[WAIT_SIZE] = "|/-\\";

static unsigned char key[16], iv[16];

/* Compile with -DDEBUG for debug mode. */
#ifndef DEBUG
#define DEBUG 0
#endif
static int debug = DEBUG;

/* restoreterm: restore initial terminal mode */
static void restoreterm(void)
{
   tcsetattr(0, 0, &termios);
}

/* print an array in hex */
static void xprint(const unsigned char *buf, size_t len)
{
   size_t i;
   for (i = 0; i < len; ++i)
   {
      printf("%02hhx", buf[i]);
   }
   putchar('\n');
}

/* generate random key and IV with feedback as to progress */
void keygen(void)
{
   int wstate = 0;
   int i = 0, err;
   int fd = open("/dev/random", O_RDONLY);
   int have_iv = 0;

   if (fd < 0)
   {
      fputs ("can't open /dev/random\n", stderr);
      exit(EXIT_FAILURE);
   }

   memset (iv, 0, 16);
   memset (key, 0, 16);

   printf ("Please move your mouse to generate entropy\n");

   /* use poll so we can update percentage done so user doesn't think program
      is hung */
   for (;;)
   {
      struct pollfd pfd = { fd, POLLIN, 0 };
      
      /* show status */
      printf ("\r%3.1f%% Generating %s... %c", i * 100.0 / 16,
	      have_iv ? "key":"IV", wait[wstate]);
      fflush (stdout);

      /* got all 16 bytes? */
      if (i == 16)
      {
	 /* stop if we have both key and iv */
	 if (have_iv)
	    break;
	 else
	 {
	    have_iv = 1;
	    i = 0;
	 }
	 putchar ('\n');
      }

      /* wait up to 1/8 second for more random data */
      if ((err = poll (&pfd, 1, 125)) == -1)
      {
	 perror("poll");
	 exit(EXIT_FAILURE);
      }

      /* if ready for reading, read in data into appropriate place */
      if (err)
      {
	 err = read (fd, have_iv ? key : iv + i, 16 - i);
	 if (err > 0)
	    i += err;
      }

      /* update our fancy wait cursor */
      wstate = (wstate + 1) & (WAIT_SIZE - 1);
   }
   close (fd);
   printf ("...done!\n");
}

int main (int argc, char *argv[])
{
   FILE *ifp, *ofp;
   unsigned char salt[256];
   char password[256];
   unsigned char kkey[MD5_DIGEST_LENGTH];
   unsigned char ekey[16]; 
   MD5_CTX ctx;
   struct termios termios_password;
   int fd, err;
   int i;
   int is_encrypt;

   /* capture current terminal mode */
   if (tcgetattr(0, &termios))
   {
      perror("tcgetattr");
      return EXIT_FAILURE;
   }

   /* restore mode on exit (in case of Ctrl-C at password prompt) */
   atexit (restoreterm);

   OpenSSL_add_all_algorithms();

   /* process command line */
   if (argc != 4 || argv[1] == NULL ||
       (strcmp (argv[1], "-e") && strcmp (argv[1], "-d")))
   {
      fputs ("usage: easyencrypt -e|-d infile outfile\n", stderr);
      return -1;
   }
   is_encrypt = !strcmp(argv[1], "-e");

   /* open input */
   ifp = fopen(argv[2], "rb");
   if (!ifp)
   {
      fputs ("cannot open input file ", stderr);
      perror (argv[2]);
      return -1;
   }

   /* open output */
   ofp = fopen(argv[3], "wb");
   if (!ofp)
   {
      fputs ("cannot open output file ", stderr);
      perror (argv[3]);
      return -1;
   }

   /* receive password from user */
   printf("Enter password to %scrypt %s: ", is_encrypt ? "en" : "de", argv[2]);
   fflush(stdout);
   memcpy(&termios_password, &termios, sizeof termios);
   termios_password.c_lflag &= ~ECHO;
   tcsetattr(0, 0, &termios_password);

   fgets(password, sizeof password, stdin);
   /* remove newline from password */
   {
      char *nl = strchr (password, '\n');
      if (nl)
	 *nl = '\0';
   }

   /* 
    * generate (on encrypt) or read (on decrypt) the random salt.
    * non-blocking random is ok for the salt
    */
   if (is_encrypt)
   {
      fd = open("/dev/urandom", O_RDONLY);
      err = read(fd, salt, sizeof salt);
      if (err != sizeof salt)
      {
	 perror ("error generating salt");
	 return -1;
      }
      close (fd);
   }
   else
   {
      fread(salt, sizeof salt, 1, ifp);
   }

   /* generate file key and iv */
   if (is_encrypt)
      keygen ();
   else
      fread (ekey, sizeof ekey, 1, ifp);

      
   /*
    * "kkey" is the password-derived key used to protect the actual file key,
    * "key".
    *
    * we use a PBKDF1-style algorithm to derive kkey from the password and
    * salt.
    */
   
   /* first round */
   MD5_Init(&ctx);
   MD5_Update(&ctx, password, strlen(password));
   MD5_Update(&ctx, salt, sizeof salt);
   MD5_Final(kkey, &ctx);
   
   /* 999 more rounds */
   for (i = 1; i < 1000; ++i)
      MD5 (kkey, MD5_DIGEST_LENGTH, kkey);

   if (is_encrypt)
   {
      /* encrypt file key. since the input is just one block, it's safe to just
       * use raw AES directly, i.e., ECB mode. */
      AES_KEY ak;
      AES_set_encrypt_key(kkey, 128, &ak);
      AES_encrypt(key, ekey, &ak);
   }
   else
   {
      /* decrypt file key */
      AES_KEY ak;
      AES_set_decrypt_key(kkey, 128, &ak);
      AES_decrypt(ekey, key, &ak);
   }
   
   if (is_encrypt)
   {
      /* write out salt */
      fwrite (salt, sizeof salt, 1, ofp);
      
      /* write out encrypted key */
      fwrite (ekey, sizeof ekey, 1, ofp);
      
      /* write out iv */
      fwrite (iv, sizeof iv, 1, ofp);
   }
   else
   {
      /* read in iv */
      fread (iv, sizeof iv, 1, ifp);
   }
   
   if (debug)
   {
      printf ("(DEBUG) password-derived key, kkey: ");
      xprint(kkey, 16);
      printf ("(DEBUG) encrypted file key, ekey: ");
      xprint(ekey, 16);
      printf ("(DEBUG) file iv: ");
      xprint(iv, 16);
      printf ("(DEBUG) file key: ");
      xprint(key, 16);
   }

   /* en/decrypt and write output */
   {
      static unsigned char inb[4096], outb[4096 + AES_BLOCK_SIZE];
      EVP_CIPHER_CTX ctx;
      int len, olen;

      EVP_CIPHER_CTX_init (&ctx);
      EVP_CipherInit_ex (&ctx, EVP_aes_128_cbc(), NULL, key, iv, is_encrypt);

      while ((len = fread(inb, 1, sizeof inb, ifp)) > 0)
      {
	 olen = sizeof outb;
	 EVP_CipherUpdate(&ctx, outb, &olen, inb, len);
	 fwrite (outb, olen, 1, ofp);
      }
      olen = sizeof outb;
      EVP_CipherFinal_ex(&ctx, outb, &olen);
      fwrite (outb, olen, 1, ofp);

      EVP_CIPHER_CTX_cleanup (&ctx);
   }

   fclose(ofp);
   fclose(ifp);
   EVP_cleanup();
   return 0;
}
