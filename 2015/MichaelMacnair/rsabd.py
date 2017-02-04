#!/usr/bin/env python2

"""
sudo apt-get install python-gmpy python-m2crypto python-crypto python-dev
sudo pip install pycurve25519
./rsabd.py >poc.key
openssl req -new -key poc.key -out poc.csr
openssl x509 -req -days 365 -in poc.csr -signkey poc.key -out poc.crt
./rsabd.py '' poc.crt
"""

import sys
import argparse
import gmpy
import logging

import curve25519

from struct import pack
from hashlib import sha256
from binascii import hexlify, unhexlify

# M2Crypto is Python 2 only
from M2Crypto import X509

from Crypto.Cipher import AES
from Crypto.PublicKey import RSA

MASTER_PUB_HEX = 'ce75b4ddd279d5f8009b454fa0f025861b65ebfbe2ada0823e9d5f6c3a15cf58'
POS = 80

# A simple AES-CTR based CSPRNG, not particularly interesting
class AESPRNG(object):
    def __init__(self, seed):
        key = sha256(seed).digest()
        self.buf      = ''
        self.buf_left = 0
        self.counter  = 0
        self.cipher   = AES.new(key, AES.MODE_ECB)

    def randbytes(self, n):
        ret = ''
        requested = n
        while requested > 0:
            # Grab all unused bytes in the buffer and then refill it
            if requested > self.buf_left:
                ret += self.buf[(16-self.buf_left):]
                requested -= self.buf_left
                # Encrypt the big-endian counter value for
                # the next block of pseudorandom bytes
                self.buf = self.cipher.encrypt(pack('>QQ', 0, self.counter))
                self.counter += 1
                self.buf_left = 16
            # Grab only the bytes we need from the buffer
            else:
                ret += self.buf[(16-self.buf_left):(16-self.buf_left+requested)]
                self.buf_left -= requested
                requested = 0
        return ret

# overwrite some bytes in orig at a specificed offset
def replace_at(orig, replace, offset):
    return orig[0:offset] + replace + orig[offset+len(replace):]

def build_key(bits=2048, e=65537, embed='', pos=1, randfunc=None):
    # generate base key
    rsa = RSA.generate(bits, randfunc)

    # extract modulus as a string
    n_str = unhexlify(str(hex(rsa.n))[2:-1])
    # embed data into the modulus
    n_hex = hexlify(replace_at(n_str, embed, pos))
    n = gmpy.mpz(n_hex, 16)
    p = rsa.p
    # compute a starting point to look for a new q value
    pre_q = n / p
    # use the next prime as the new q value
    q = pre_q.next_prime()
    n = p * q
    phi = (p-1) * (q-1)
    # compute new private exponent
    d = gmpy.invert(e, phi)
    # make sure that p is smaller than q
    if p > q:
        (p, q) = (q, p)
    return RSA.construct((long(n), long(e), long(d), long(p), long(q)))

def recover_seed(key='', modulus=None, pos=1):
    # recreate the master private key from the passphrase
    #master = bytes(sha256(key).digest())
    # or just reuse hashed master
    master = bytes(unhexlify(MASTER_PUB_HEX))
    # extract the ephemeral public key from modulus
    ephem_pub = modulus[pos:pos+32]
    # compute seed with master private and ephemeral public
    return (curve25519.shared(master,ephem_pub), ephem_pub)

def generate_new_seed():
    # deserialize master ECDH public key embedded in program
    master_pub = curve25519.public(unhexlify(MASTER_PUB_HEX))
    # generate a random (yes, actually random) ECDH private key
    ephem = curve25519.genkey()
    # derive the corresponding public key for later embedding
    ephem_pub = curve25519.public(ephem)
    # combine the ECDH keys to generate the seed
    seed = curve25519.shared(ephem,master_pub)

    return seed, ephem_pub

def generate_from_seed(seed, ephem_pub):
    prng = AESPRNG(seed)
    ephem_pub = bytes(ephem_pub)

    # deterministic key generation from seed
    logging.debug("Seed: {}".format(hexlify(seed)))
    logging.debug("Embedding public key: {}".format(hexlify(ephem_pub)))
    k= build_key(embed=ephem_pub, pos=POS, randfunc=prng.randbytes)
    logging.debug("Resulting modulus: {:X}".format(k.n))
    return k

def generate_new_key():
    (seed, ephem_pub) = generate_new_seed()
    return generate_from_seed(seed, ephem_pub)

def get_private_from_modulus(modulus, pos=POS):
    orig_modulus = unhexlify(modulus)
    (seed, ephem_pub) = recover_seed(key=None, modulus=orig_modulus, pos=pos)
    rsa = generate_from_seed(seed, ephem_pub)

    if long(hexlify(orig_modulus), 16) != long(rsa.n):
        raise Exception("key recovery failed\noriginal modulus:   {}\nrecovered seed: {}\ncalculated modulus: {:X}\n".format(modulus,hexlify(seed),long(rsa.n)))

    return rsa

def get_modulus_from_gpg(cert):
    import pgpdump, types
    try:
        data = pgpdump.BinaryData(cert)
    except pgpdump.utils.PgpdumpException:
        data = pgpdump.AsciiData(cert)

    public_gpg_keys = [packet for packet in data.packets() if type(packet) == pgpdump.packet.PublicKeyPacket]
    if len(public_gpg_keys) != 1:
        raise(Exception("Found {} gpg public keys, require exactly 1, aborting.".format(len(public_gpg_keys))))
        
    return public_gpg_keys[0].modulus
        

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate and reverse backdoor-ed RSA keys. Output private key on stdout.')
    parser.add_argument('--generate', action='store_true', help='Generate a new backdoor-ed key (default: reverse an existing certificate)')
    parser.add_argument('--gpg', action='store_true', help='Generate a new backdoor-ed key and output p & q')
    parser.add_argument('--debug', action='store_true', help='Print debug info to stderr')
    parser.add_argument('certificate', nargs='?', type=argparse.FileType('r'), help='Certificate file containing the key to reverse, if --generate is not specified (cert can alternatively be passed on stdin).')
    args = parser.parse_args()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.INFO)

    if args.generate:
        rsa = generate_new_key()
    else: # reverse
        # Get certificate
        if args.certificate:
            certstr = args.certificate.read()
        else:
            certstr = sys.stdin.read()

        # Parse input as x.509 certificate
        if args.gpg:
            modulus = "{:X}".format(get_modulus_from_gpg(certstr))
        else:
            x509 = X509.load_cert_string(certstr)
            modulus = x509.get_pubkey().get_modulus()

        # Reverse
        rsa = get_private_from_modulus(modulus)

    if args.generate and args.gpg:
        logging.debug("p: 00{:X}".format(rsa.p))
        logging.debug("q: 00{:X}".format(rsa.q))
        print("00{:X}".format(rsa.p))
        print("00{:X}".format(rsa.q))
    else:
        print rsa.exportKey()
