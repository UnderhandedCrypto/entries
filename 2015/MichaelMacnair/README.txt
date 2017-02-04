README.txt

NAME OF PROJECT: Kleptographic Key Generation
WAS THIS A TEAM PROJECT: No
PROJECT LICENSE: Apache 2.0

CHALLENGE (GPG KEY LEAK / PASSWORD HASHING BACKDOOR): GPG KEY LEAK

DESCRIPTION:
The RSA key generation routine of libgcrypt (the crypto library used by GPG) is
replaced with an algorithm that generates a back-doored key, allowing the
private key to be derived from the public key.

This key is cryptographically strong and cannot be differentiated from a normal
key except by the holder of a master private key. However, this master private
key holder can derive the private key from the public key.

Example run:
    $ # Victim generates a new GPG key (using backdoor-ed libgcrypt)
    $ LD_PRELOAD=./libgcrypt.so gpg --gen-key
        gpg (GnuPG) 2.0.18; Copyright (C) 2011 Free Software Foundation, Inc.
        ...
        Please select what kind of key you want:
           (4) RSA (sign only)
        Your selection? 4
        What keysize do you want? (2048) 
        Requested keysize is 2048 bits   
        ...
        backdoored key generated
        ...
        public and secret key created and signed.

    $ # victim exports the public key as per usual for keyservers etc
    $ gpg --export --armour mynewkey > bobs-pubkey.asc

    $ # attacker / backdoor owner obtains bobs-pubkey.asc from keyserver
    $ rsabd.py --gpg bobs-pubkey.asc
        -----BEGIN RSA PRIVATE KEY-----
        MIIEpAIBAAKCAQEAr2cfcUghZLSc+gB3DcoEDb8aYsQfJR7ruiGKPaXEMRNh+DkZ
        rp6cSNpJngX3sMr0MhpY1Cw8Ox2CT0RAf0exuIEStOvbF8qxM4SraawSPwnEgSIW
        mhW5KvcdUmwSCtA0r73kjuHJHrLQG8ttpkmwLq8fUFswcNjn4pA603twH1plmbqG
        sZWXuNJZNWf7xmrTLXeui5UJY7dYsoaLtYS75mJBGhhJlQf6+ixjOMbrVGGonQOP
        ...
        z3dMH54xXSUWpE0/O3JHq5/qS+xGvF731vCJwkg44RiHh0cls1b2EA==
        -----END RSA PRIVATE KEY-----


Setup (attacker):
 - Create master Curve25519 public/private keypair
 - backdoor libgcrypt, including master public key
 - get backdoored libgcrypt on victim machines

Backdoored key generation algorithm (libgcrypt on victim):
 - Generate an ephemeral Curve25519 key
 - Do an ECDH key exchange with the master public key and the ephemeral private
   key, to generate a shared secret, called seed.
    seed = Curve25519(ephem_private, master_public)
 - Seed a PRNG with this shared secret seed
 - Generate an RSA key using this PRNG in the normal manner.
 - (the clever bit starts here) *overwrite* the middle parts of the RSA key's
   modulus with the ephemeral public key. Note that a Curve25519 public key
   looks like 256 bits of uniformly random data - this is an explicit design goal
   of Curve25519.
 - (Discard ephemeral public/private keypair.)
 - Go backwards from the now corrupted modulus:
     almost_q = n / p
 - This is very unlikely to make a valid RSA key, so find a nearby one:
     new_q = next_prime(almost_q)
     new_n = p * new_q
 - This new RSA key (p, new_q, new_n) is valid, cryptographically strong, and
   the middle of the modulus contains the ephemeral 25519 public key (because
   the next prime along wasn't very far away, so the more significant bits of the
   modulus that were overwritten haven't changed)

Private key recovery algorithm (attacker):
 - Extract the ephemeral Curve 25519 public key from the middle of the GPG RSA
   public key's modulus.
 - Generate the shared secret:
     seed = Curve25519(ephem_public, master_private)
 - Repeat the key generate process described above

Implementation notes:
 - a small patch to libgcrypt-1.5.4 that calls out to a python script, rsabd.py
 - tested against gpg 2.0.18
 - rsabd.py, from https://github.com/msm-/tls-kleptography in turn
   originally from ryancdotorg
 - to run it, first install the dependencies listed at the top of rsabd.py

TODO list to productionise this backdoor:
 - replace python dependency with native libgcrypt implementation
 - support for keys other than RSA 2048
 - remove "backdoor" message...
 - have rsabd.py product gpg format private keys, not PKCS#1

Credit:
 - ryancdotorg for the original rsabd.py
