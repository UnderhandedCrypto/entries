#include "tinyaesctr.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

static uint8 state[4][4];
static uint8 *out;
static int excess;

#define State(j,i) state[j][i]

extern const uint8 aes_sbox[];
extern const uint8 aes_Rcon[];
// The array that stores the round keys.
static uint8 RoundKey[176];

// The Key input to the AES Program
uint8 buf[KLEN];

static void KeyExpansion(void);
static void Cipher(void);
static void aes_setup(boolean key_expanded, boolean generate_iv, boolean reset_iv);

void increment_ctr(void)
{
        for (int i = KLEN - 1; i; i--)
                if (++buf[i])
                        break;
}


// process data in place, can be used as a single call with
// MODE_NEWKEY|MODE_GENIV|MODE_DONE
void aes_ctr_crypt(uint8 data[], int len, unsigned mode)
{
        aes_setup((mode & MODE_KEEPKEY) || !(mode & MODE_SETKEY), mode & MODE_GENIV, mode & MODE_RESETIV);
        /* encryption and decryption are the same in CTR mode. */
        if (mode & (MODE_ENCRYPT | MODE_DECRYPT)) {
                int i, j;
                uint8 outbuf[KLEN];
                out = outbuf;
                int offset = 0;
                if (excess) {
                        int td = MIN(excess, len);
                        /* if we rekeyed, we need to rerun the Cipher, else we
                         * can use the old state */
                        aes_setup(mode & MODE_KEEPKEY, mode & MODE_GENIV, mode & MODE_RESETIV);
                        if(mode&MODE_REKEY)
                                Cipher();
                        else 
                                for (i = 0; i < 4; ++i) {
                                        for (j = 0; j < 4; ++j) {
                                                outbuf[(i * 4) + j] = state[j][i];
                                        }
                                }
                        for (int i = 0; i < td; i++)
                                data[i] ^= outbuf[i + KLEN - td];
                        len -= td;
                        offset += td;
                        if (len >= 0)
                                increment_ctr();
                }
                while (len > 0) {
                        Cipher();
                        for (int i = 0; i < MIN(KLEN, len); i++)
                                data[i + offset] ^= outbuf[i];
                        offset += KLEN;
                        len -= KLEN;
                        if (len >= 0)
                                increment_ctr();
                }
                excess = -len;
        }
}

/* perform aes block setup */
static void
aes_setup(boolean key_expanded, boolean generate_iv, boolean reset_iv)
{
        if (!key_expanded) {
                KeyExpansion();
        }
        if (generate_iv)
                fill_random();
        /* RFC 3686 requires the block counter start at 1 */
        if (reset_iv)  {
                buf[KLEN - 1] = 1;
                buf[KLEN - 2] = 0;
                buf[KLEN - 3] = 0;
                buf[KLEN - 4] = 0;
        }
}


/*****************************************************************************/
/* Defines:                                                                  */
/*****************************************************************************/
// The number of columns comprising a state in AES. This is a constant in AES. Value=4
#define Nb 4
// The number of 32 bit words in a key.
#define Nk 4
// Key length in bytes [128 bit]
#define keyln 16
// The number of rounds in AES Cipher.
#define Nr 10
/*****************************************************************************/
/* Private functions:                                                        */
/*****************************************************************************/
static uint8 getSBoxValue(uint8 num)
{
        return aes_sbox[num];
}
// This function produces Nb(Nr+1) round keys. The round keys are used in each round to decrypt the states.
static void KeyExpansion()
{
        uint32 i, j, k;
        uint8 tempa[4]; // used for the column/row operations
        // The first round key is the key itself.
        for (i = 0; i < Nk; ++i) {
                RoundKey[(i * 4) + 0] = buf[(i * 4) + 0];
                RoundKey[(i * 4) + 1] = buf[(i * 4) + 1];
                RoundKey[(i * 4) + 2] = buf[(i * 4) + 2];
                RoundKey[(i * 4) + 3] = buf[(i * 4) + 3];
        }
        // All other round keys are found from the previous round keys.
        for (; (i < (Nb * (Nr + 1))); ++i) {
                for (j = 0; j < 4; ++j) {
                        tempa[j] = RoundKey[(i - 1) * 4 + j];
                }
                if (i % Nk == 0) {
                        // This function rotates the 4 bytes in a word to the left once.
                        // [a0,a1,a2,a3] becomes [a1,a2,a3,a0]
                        // Function RotWord()
                        {
                                k = tempa[0];
                                tempa[0] = tempa[1];
                                tempa[1] = tempa[2];
                                tempa[2] = tempa[3];
                                tempa[3] = k;
                        }
                        // SubWord() is a function that takes a four-byte input word and
                        // applies the S-box to each of the four bytes to produce an output word.
                        // Function Subword()
                        {
                                tempa[0] = getSBoxValue(tempa[0]);
                                tempa[1] = getSBoxValue(tempa[1]);
                                tempa[2] = getSBoxValue(tempa[2]);
                                tempa[3] = getSBoxValue(tempa[3]);
                        }
                        tempa[0] =  tempa[0] ^ aes_Rcon[i / Nk];
                } else if (Nk > 6 && i % Nk == 4) {
                        // Function Subword()
                        {
                                tempa[0] = getSBoxValue(tempa[0]);
                                tempa[1] = getSBoxValue(tempa[1]);
                                tempa[2] = getSBoxValue(tempa[2]);
                                tempa[3] = getSBoxValue(tempa[3]);
                        }
                }
                RoundKey[i * 4 + 0] = RoundKey[(i - Nk) * 4 + 0] ^ tempa[0];
                RoundKey[i * 4 + 1] = RoundKey[(i - Nk) * 4 + 1] ^ tempa[1];
                RoundKey[i * 4 + 2] = RoundKey[(i - Nk) * 4 + 2] ^ tempa[2];
                RoundKey[i * 4 + 3] = RoundKey[(i - Nk) * 4 + 3] ^ tempa[3];
        }
}
// This function adds the round key to state.
// The round key is added to the state by an XOR function.
static void AddRoundKey(uint8 round)
{
        uint8 i, j;
        for (i = 0; i < 4; i++) {
                for (j = 0; j < 4; ++j) {
                        State(j, i) ^= RoundKey[round * Nb * 4 + i * Nb + j];
                }
        }
}
// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
static void SubBytes(void)
{
        uint8 i, j;
        for (i = 0; i < 4; ++i) {
                for (j = 0; j < 4; ++j) {
                        State(i, j) = getSBoxValue(State(i, j));
                }
        }
}
// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
static void ShiftRows(void)
{
        uint8 temp;
        // Rotate first row 1 columns to left
        temp        = state[1][0];
        state[1][0] = state[1][1];
        state[1][1] = state[1][2];
        state[1][2] = state[1][3];
        state[1][3] = temp;
        // Rotate second row 2 columns to left
        temp        = state[2][0];
        state[2][0] = state[2][2];
        state[2][2] = temp;
        temp = state[2][1];
        state[2][1] = state[2][3];
        state[2][3] = temp;
        // Rotate third row 3 columns to left
        temp = state[3][0];
        state[3][0] = state[3][3];
        state[3][3] = state[3][2];
        state[3][2] = state[3][1];
        state[3][1] = temp;
}
static uint8 xtime(uint8 x)
{
        return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}
// MixColumns function mixes the columns of the state matrix
static void MixColumns()
{
        uint8 i;
        uint8 Tmp, Tm, t;
        for (i = 0; i < 4; ++i) {
                t   = state[0][i];
                Tmp = state[0][i] ^ state[1][i] ^ state[2][i] ^ state[3][i] ;
                Tm  = state[0][i] ^ state[1][i] ; Tm = xtime(Tm); state[0][i] ^= Tm ^ Tmp ;
                Tm  = state[1][i] ^ state[2][i] ; Tm = xtime(Tm); state[1][i] ^= Tm ^ Tmp ;
                Tm  = state[2][i] ^ state[3][i] ; Tm = xtime(Tm); state[2][i] ^= Tm ^ Tmp ;
                Tm  = state[3][i] ^ t ; Tm = xtime(Tm); state[3][i] ^= Tm ^ Tmp ;
        }
}

// Cipher is the main function that encrypts the PlainText.
static void Cipher()
{
        uint8 i, j, round = 0;
        //Copy the input PlainText to state array.
        //memcpy(state,in,KLEN);
        for (i = 0; i < 4; ++i) {
                for (j = 0; j < 4 ; ++j) {
                        state[j][i] = buf[(i * 4) + j];
                }
        }
        // Add the First round key to the state before starting the rounds.
        AddRoundKey(0);
        // There will be Nr rounds.
        // The first Nr-1 rounds are identical.
        // These Nr-1 rounds are executed in the loop below.
        for (round = 1; round < Nr; ++round) {
                SubBytes();
                ShiftRows();
                MixColumns();
                AddRoundKey(round);
        }
        // The last round is given below.
        // The MixColumns function is not here in the last round.
        SubBytes();
        ShiftRows();
        AddRoundKey(Nr);
        // The encryption process is over.
        // Copy the state array to output array.
        for (i = 0; i < 4; ++i) {
                for (j = 0; j < 4; ++j) {
                        out[(i * 4) + j] = state[j][i];
                }
        }
}
