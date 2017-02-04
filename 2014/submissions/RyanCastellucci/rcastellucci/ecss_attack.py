#!/usr/bin/env python
# coding=utf8

from ecss import *

def recover_private(ss):
    v, r1, s2 = decode_selfsig(ss)

    # recompute z1
    z1_str = sha256(chr(v) + encode(r1,256,32) + encode(s1,256,32))
    z1 = hash_to_int(z1_str)

    # recover public key Q
    Q = compress(ecdsa_raw_recover(z1_str, (v,r1,s1)))

    # compute c and z2 from Q
    c  = hash_to_int(sha256(unhexlify(Q)+"\x01"))
    z2 = hash_to_int(sha256(unhexlify(Q)+"\x02"))
    z2_str = encode(z2,256,32)

    # recover original y1
    beta = pow(r1*r1*r1+A*r1+B,(P+1)/4,P)
    y1 = beta if ((beta + v) % 2) else (P-beta)

    # compute public coordinates for c
    Cx, Cy = base10_multiply(G, c)

    # compute the public coordinates for k2 using public point
    # addition without knowing k2 or being able to compute it
    r2, y2 = base10_add((r1, y1), (Cx, Cy))

    # recovery for reused k is (z1*s2 - z2*s1)/(r*(s1-s2))
    # recovery for known difference between k1 and k2 is
    #      r2*z1 + r1*(c*s2-z2)
    # k1 = --------------------
    #         s1*r2 - s2*r1
    #
    # or, in a slightly expanded form
    #
    #      r2*z1 + r1*c*s2 - r1*z2
    # k1 = -----------------------
    #           s1*r2 - s2*r1

    t1 = (s1 * r2) % N
    t2 = (s2 * r1) % N
    denom = (t1 - t2) % N

    t3 = (r2 * z1) % N
    t4 = (c * s2 - z2) % N
    t5 = (t4 * r1) % N
    numer = (t3 + t5) % N

    k1 = (numer * inv(denom, N)) % N

    d = inv(r1, N) * (s1 * k1 - z1) % N

    return (encode(d,16,64), k1)

ss = b64_to_hex(argv[1])
vr1s2 = decode_selfsig(ss)

Q = ecdsa_selfverify(ss)

if Q:
    d, k1 = recover_private(ss)
    print 'd:  %s' % d
    print 'Q:  %s' % Q
else:
    print 'Q:  VERIFICATION FAILED'

# vim: sw=4 ts=4 et ai si
