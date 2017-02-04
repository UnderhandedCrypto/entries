#!/usr/bin/env python
# coding=utf8

from sys import argv
from math import log, ceil, floor
from binascii import hexlify, unhexlify
from base64 import b64encode, b64decode

from ec import *

def b64_to_hex(b):
    return hexlify(b64decode(b))

def hex_to_b64(h):
    return b64encode(unhexlify(h))

# we need an arbitrary fixed 256 bit integer for s1, use digits of pi
s1 = 31415926535921246614439024146974356232736948933844281113370478935789350942407

# Based on SEC1v2 §4.1.7, but produces a more rigorous self-signature
def ecdsa_raw_selfsign(seed):
    # 256 bit integer (ephemeral private key) k1 from the result of sha256(seed)
    k1 = hash_to_int(sha256(seed))

    # compute public coordinates for k
    r1, y1 = base10_multiply(G, k1) # point(r, y) = k1*G
    v = 27 + (y1 % 2)

    # 256 bit integer z1 from the result of sha256(v | r1 | s1)
    z1_str = sha256(chr(v) + encode(r1,256,32) + encode(s1,256,32))
    z1 = hash_to_int(z1_str)

    # derive the private key, working backwards from the signature
    d = inv(r1, N) * (s1 * k1 - z1) % N
    #assert s1 == inv(k1, N) * (z1 + r1 * d) % N

    # Q is the encoded public key of d - this can also be
    # computed with the recover operation, but that is slow
    Q = encode_pubkey(base10_multiply(G, d), 'hex_compressed')
    #assert Q == ecdsa_raw_recover(z1_str, (v,r1,s1))
    
    ### extra steps vs SEC1v2 §4.1.7 - sign a hash of Q with derived
    ### private key d using the previously determined k

    # compute c and z2 from Q
    c  = hash_to_int(sha256(unhexlify(Q)+"\x01"))
    z2 = hash_to_int(sha256(unhexlify(Q)+"\x02"))

    # get a new ephemeral private key derived from k1 and c
    k2 = add_privkeys(k1, c)
    # compute the public coordinates for k2
    r2, y2 = base10_multiply(G, k2)

    # the signature
    s2 = inv(k2, N) * (z2 + r2 * d) % N

    # s1 is omitted because it's a constant, r2 can be
    # derived from r1 and is therefore omitted as well
    return (v, r1, s2, encode(d,16,64), Q)

def encode_selfsig(v, r1, s2):
    return hexlify(chr(v) + encode(r1,256,32) + encode(s2,256,32))

def ecdsa_selfsign(seed):
    v, r1, s2, d, Q = ecdsa_raw_selfsign(seed)
    vr1s2 = encode_selfsig(v, r1, s2)
    return (vr1s2, d, Q)

def ecdsa_raw_selfverify(v, r1, s2):
    # recompute z1
    z1_str = sha256(chr(v) + encode(r1,256,32) + encode(s1,256,32))
    z1 = hash_to_int(z1_str)

    # recover public key Q
    try:
        Q = compress(ecdsa_raw_recover(z1_str, (v,r1,s1)))
    except:
        # might get an exception trying to process an invalid self signature
        return None

    # compute c and z2 from Q
    c  = hash_to_int(sha256(unhexlify(Q)+"\x01"))
    z2 = hash_to_int(sha256(unhexlify(Q)+"\x02"))

    # recover original y1
    beta = pow(r1*r1*r1+A*r1+B,(P+1)/4,P)
    y1 = beta if ((beta + v) % 2) else (P-beta)

    # compute public coordinates for c
    Cx, Cy = base10_multiply(G, c)

    # compute the public coordinates for k2 using public point
    # addition without knowing k2 or being able to compute it
    r2, y2 = base10_add((r1, y1), (Cx, Cy))

    # and actually verify the self-signature
    return ecdsa_raw_verify(encode(z2,256,32), (v, r2, s2), Q) and Q or None

def decode_selfsig(ss):
    v = decode(ss[0:2],16)
    r1 = decode(ss[2:66],16)
    s2 = decode(ss[66:130],16)
    return (v, r1, s2)

def ecdsa_selfverify(ss):
    v, r1, s2 = decode_selfsig(ss)
    return ecdsa_raw_selfverify(v, r1, s2)

# vim: sw=4 ts=4 et ai si
