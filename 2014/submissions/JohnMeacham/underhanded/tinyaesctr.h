#ifndef TINYAESCTR_H
#define TINYAESCTR_H

#define MODE_ENCRYPT 0x0001
#define MODE_DECRYPT 0x0002
#define MODE_SETIV   0x0010
#define MODE_GENIV   0x0020
#define MODE_RESETIV 0x0040
#define MODE_SETKEY  0x0100
#define MODE_REKEY   0x0200
#define MODE_KEEPKEY 0x0400

#define TRUE 1
#define FALSE 0
typedef unsigned char boolean;
typedef unsigned char uint8;
/* these should be modified appropriately for target system */
typedef unsigned short uint16;
typedef unsigned int uint32;

/* key and block length are both fixed at 128 bits */
#define KLEN 16

// static area used to pass keys and iv
extern uint8 buf[KLEN];

/* The various modes. These can be combined to perform a full encryption in a
 * single call.
 *
 * MODE_ENCRYPT - encrypt data
 * MODE_DECRYPT - decrypt data (the same as ENCRYPT for CTR mode)
 * MODE_KEEPKEY - indicates a continuation of the same encrypted stream.
 * MODE_SETKEY  - explicitly set key from buf
 * MODE_GENIV   - generate a new random IV and store it in buf
 * MODE_SETIV   - set IV to buf
 * MODE_RESETIV - reset IV to initial RFC3686 value based on buf
 * MODE_REKEY   - rekey stream with new key from buf
 */

void aes_ctr_crypt(uint8 data[], int len, unsigned mode);

/* If generated IVs are used then this should be provided by the user of the
 * algorithm and fill buf with 128 bits of cryptographically secure random data
 */
void fill_random(void);

/* example modes
 *
 * perform full encryption in a single pass, reading key from buf, generating a new IV and putting it in buf.
 * aes_ctr_crypt(data,len,MODE_ENCRYPT|MODE_SETKEY|MODE_GENIV)
 *
 * manually setting key and IV then using them.
 * aes_ctr_crypt(NULL, 0, MODE_SETKEY);
 * aes_ctr_crypt(NULL, 0, MODE_SETIV);
 * aes_ctr_crypt(data,len,MODE_ENCRYPT|MODE_KEEPKEY);
 */

#endif
