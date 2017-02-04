NAME OF PROJECT: LEAKYTAP
WAS THIS A TEAM PROJECT: No
PROJECT LICENSE: AGPL-V3

CHALLENGE: GPG KEY LEAK

NOTE: The project description should explain your entry
      clearly; including the techniques used, why it works,
      why you think it would go undetected. It may be a
      summary of a longer document included in the /submission/
      folder. This description will eventually be published on
      the Underhanded Crypto Contest web site, along with the
      contents of the /submission/ folder. This must clearly
      describe the entry to the judges; to respect the time of
      our judges, they will not take the time to hunt for the
      techniques used. The description should be 500 to 2000
      words.

DESCRIPTION:

See submission/readme.md for more details.

# Backdoor description

LEAKYTAP is a proof-of-concept GPG backdoor that aims to leak the target user's private key by using the GPG session key as a subliminal channel. The backdoor is currently limited to targeting RSA public keys only but can be modified to support other GPG public key types as well. The backdoor was designed with a certain degree of covertness with respect to the average GPG user in mind but does not provide strong properties such as confidentiality with forward-secrecy or deniability to the attacker in the face of a more advanced user. The use-case i had in mind when designing the backdoor was an attacker intercepting some security-related embedded device en-route to the target modifying its firmware to include the backdoored GPG executable (hence the choice for writing the patch for GnuPG classic) in order to learn the target user's private key but it is certainly not limited to such a scenario.

Consider a n-bit public modulus N (with n/2-bit prime factors p and q) and a 256-bit (the standard and maximum in GPG) symmetric session key, an attacker Mallory and a target Alice. The backdoor works as follows:

1. Mallory seeks to obtain the private key corresponding to Alice's public key and manages to backdoor Alice's GPG executable with her backdoor.
2. Mallory triggers the backdoor by sending a message with a specially crafted session key (see below) encrypted with Alice's public key to Alice
3. When Alice decrypts Mallory's message, the private key corresponding to the public key it was encrypted with becomes accessible to GPG. A fragment (the size of the channel bandwith) of the prime factor p is extracted and encrypted by the attacker's symmetric key embedded in the backdoor, using an IV derived from Mallory's session key. This encrypted fragment is stored in a temporary file.
4. When Alice sends a response message (to anyone, not only Mallory), the backdoor retrieves the currently stored encrypted fragment and sets the session key to it. Thus, once Alice responds to Mallory the latter can extract a leaked fragment of the prime factor p.
5. Mallory repeats this process as many times as necessary (depending on channel bandwith and prime factor size) until she can recover Alice's private key.

## Mallory's trigger session keys

Mallory's session keys are formatted as follows:

>*[I|IV|G]*

Where I is an *index byte*, IV the randomized 128-bit IV and 120 bits of random garbage G. The *index byte* indicates what fragment of the prime factor is to be leaked next and does so by falling in one of several 'buckets' (ie. byte ranges) determined by the prime factor size and channel bandwith. For example, assume we have 1024-bit prime factors and 256-bit session keys, then we need to leak p in 4 fragments hence we can express the indexes {0,..,3} as the ranges {{0x00,..,0x3F},{0x40,..,0x7F},..}. Hence if mallory wants to leak the 256 uppermost bits of p she sets I = random(0x00,0x3F).

## Alice's response session keys

When there is an 'unretrieved leak' (ie. Mallory has sent a trigger message but Alice has not responded to Mallory yet) Alice's response session keys consist of fragments of prime factor p encrypted using AES-256 in CBC mode with the attacker's embedded symmetric key and the IV specified by the last trigger message from Mallory. A 3rd party Bob communicating with Alice has a chance of backdoor discovery if there is an unretrieved leak and Alice sends more than one message to Bob. This is the case since as long as the leak goes unretrieved (ie. Mallory sends no new trigger message), the session key chosen by alice is static and Bob will notice identical session keys for different messages. This could be addressed by the attacker by including a random nonce (at the cost of reduced channel bandwith) with the leaked fragment and encrypting it right before sending it instead of before storage.

## Partial leaking and key reconstruction

Should Alice break of communication with Mallory (or Mallory choose to limit backdoor interaction) before the entire prime factor is recovered it is still possible for Mallory to reconstruct Alice's private key from partial prime factor exposure. It is known from reseach regarding partial key exposure attacks [1](http://crypto.stanford.edu/~dabo/papers/RSA-survey.pdf) [2](https://eprint.iacr.org/2008/510.pdf) that efficient reconstruction of the private key is possible given access to the n/4 (where n is the modulus bitsize) most or least significant bits of a modulus prime factor (or possibly less using eg. techniques proposed in [3](https://www.usenix.org/legacy/event/sec08/tech/full_papers/halderman/halderman.pdf)) or ~27% of any of the private exponent bits at random. This can be used by Mallory by reducing the protocol run to an exchange of 2 (for 2048-bit keys) or 4 (for 4096-bit keys) GPG messages.

## Example protocol run

Below is an example backdoor protocol run:

* Mallory: session_key = [I0|IV0|G0]
* Alice: session_key = E(p[0:256], K, IV0)
* Mallory: session_key = [I1|IV1|G1]
* Alice: session_key = E(p[256:512], K, IV1)
* Mallory: session_key = [I2|IV2|G2]
* Alice: session_key = E(p[512:768], K, IV2)
* Mallory: session_key = [I3|IV3|G3]
* Alice: session_key = E(p[768:1024], K, IV3)

# Design Rationale

The backdoor was designed with the average GPG user in mind (as a target) and as such focuses on the following:

* Avoiding suspicious 'out of context' passphrase queries or long-term keylogging by the backdoored GPG in order to access encrypted private keys
* Avoiding modifications to public keys
* Avoiding Low-bandwith channels requiring intensive interaction
* Guaranteeing exclusive backdoor usage by the attacker

Obviously, the backdoor does not achieve such properties as undetectability and deniability but is probably still sufficiently covert to bypass average scrutiny.

## Confidentiality

While Alice broadcasts leaked data to all parties she communicates with, only those parties in posession of both the IVs and attacker's embedded secret key can use the backdoor. Given that access to the secret key is predicated upon access to the backdoor and access to the IVs predicated upon either access to the backdoor or Alice's private key this guarantees exclusive backdoor usage to the attacker.

## (Un)detectability

The idea is that while public keys are commonly scrutinized by the average security-conscious user this does not go for the, essentially 'throwaway', session keys. Embedding our subliminal channel in the session keys has the added benefit of being able to retroactively copromise preexisting public keys rather than, for example, only those generated with a backdoored key generation mechanism (such as many of the so-called [SETUP attacks](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.54.616&rep=rep1&type=pdf)). Similarly opting for a high-bandwith channel by leaking as much of the private key as we can per backdoor interaction limits the total amount of interaction (translating to eg. fewer GPG e-mails requiring a response) which might arouse suspicion in the average user. 

Mallory's triggering session keys are virtually indistinguishable from random session keys since 248 out of 256 bits (IV + garbage) are randomly generated and while the 8 most significant bit indicate the fragment to leak they do so in a rather inconspcious fashion (see above).

More problematic are Alice's leaking session keys. It should be noted that anyone can 'trigger' the backdoor insofar that the session key of any party communicating with Mallory is treated as a 'trigger' and subsequently Alice's session keys are derived from this leak. The temp storage used to hold a leak triggered by the last message received by Alice only gets updated upon decrypting new messages. Hence any messages sent by Alice without intermittent decryption (of messages by any party, not just Mallory) result in identical session keys, something that might be noticed by any party (or collaborating parties) receiving multiple messages from Alice in such a case. The attacker could mitigate this by including a nonce (in the fashion noted above) at the price of reduced channel bandwith.

Finally, the rather blunt way used to store encrypted leaked fragments (in a temporary file) might arouse some suspicion as well though modifying this part to achieve additional covertness should not prove difficult.

## Deniability

The backdoor offers some deniability to Mallory since instead of leaking exclusively to Mallory the backdoor leaks to all communicating parties. In order to prove Mallory was the party interacting with the backdoor Alice would have to retain a backarchive of Mallory's GPG messages to her so that upon discovery of the backdoor she could extract the encrypted leak fragment from the temporary storage, try all candidate IVs in Mallory's trigger session keys (in combination with the extracted backdoor secret key) and check whether the result is part of a prime factor of her private key. While this is certainly plausible and requires relatively little effort, it at least complicates matters for those users who practice 'burn after reading' or in those setups where GPG usage is automated and merely the plaintext content is processed/stored while the original ciphertext is discarded.

## Reliability

The backdoor suffers from some reliability issues arising from the fact that any session key is treated as a trigger key and hence there is the possibility that Alice decrypts a 3rd party's message between decrypting Mallory's trigger and sending a response, thus leading to Mallory retrieving corrupted plaintext. This can be addressed by, upon detecting corruption, executing several protocol iterations and recombining the recovered pieces (with some possible redundancy). The number of re-tries depends on Alice's communication practices (immediate response or not, etc.), number of parties Alice is in communication with and communication frequency. All in all this does not seem to be a major obstacle to backdoor usability.