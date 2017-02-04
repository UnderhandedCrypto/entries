"""
Super-comedy DSA.

You'd have to be mental to use this for anything.
"""

import random
from hashlib import sha1, sha256

def extended_euclidian(a, b):
    if b == 0:
        return (1, 0)
    assert a > 0 and b > 0

    q, r = divmod(a, b)
    s, t = extended_euclidian(b, r)

    return t, s - q * t

def invmod(x, y):
    ax, by = extended_euclidian(x, y)
    while ax < 0:
        ax += y
    return ax

class group(object):
    def __init__(self, p, q, g):
        self.p = p
        self.q = q
        self.g = g

def gen_pair(group):
    x = random.randrange(1, group.q - 1)
    y = pow(group.g, x, group.p)
    return (group, y), (group, x)

def hash(hh, msg):
    return long(hh(msg).hexdigest(), 16)
    
def sign_sha1(priv, msg):
    group, x = priv
    h = hash(sha1, msg)
    
    while True:
        k = random.randrange(1, group.q - 1)
        r = pow(group.g, k, group.p) % group.q
        if r == 0:
            continue
        
        kinv = invmod(k, group.q)
        s = (kinv * (h + x * r)) % group.q
        if s == 0:
            continue
        return (r, s)

def verify_sha1(pub, sig, msg):
    verify(pub, sig, hash(sha1, msg))

def verify(pub, sig, h):
    group, y = pub
    r, s = sig
    
    if not (r > 0 and r < group.q and s > 0 and s < group.q):
        raise ValueError, 'invalid dsa signature'
    
    w = invmod(s, group.q)
    u1 = (h * w) % group.q
    u2 = (r * w) % group.q
    v = (pow(group.g, u1, group.p) * pow(y, u2, group.p)) % group.p
    v %= group.q
    
    if v != r:
        raise ValueError, 'invalid dsa signature'

def prepare_recover_x(group, sig):
    r, s = sig
    rinv = invmod(r, group.q)
    return (rinv,)

def recover_x_given_sig_k(group, k, sig, h, precomp = None):
    r, s = sig
    if precomp is None:
        precomp = prepare_recover_x(group, sig)
    rinv, = precomp
    x = ((s * k - h) * rinv) % group.q
    return x
