#!/usr/bin/env python2


'''
CKDF: Collatz-based Key Derivation Function.

See the `How to use` section at the end of this file for quick start;
detailed documentation at http://r.rogdham.net/kNBkwa


Copyheart Rogdham, 2014 ~ Sharing is caring... Please copy!
See http://copyheart.org/ for more details.

Licence of this file: CC0

    This program is free software. It comes without any warranty, to the extent
    permitted by applicable law. You can redistribute it and/or modify it under
    the terms of the CC0 1.0 Universal Licence (French).
    See https://creativecommons.org/publicdomain/zero/1.0/deed.fr for more
    details.

Tested with Python 2.7.
'''


import hashlib
import os
import struct


# Utility functions

def _hash(x):
    '''Hash of x, using the (still quite) secure sha512 function'''
    return hashlib.sha512(x).digest()


def _generate_salt():
    '''Generate 64 cryptographically secure bytes'''
    return os.urandom(64)


# CKDF core functions

def __ckdf_core(user, password, salt):
    '''Used for both key derivation and verification'''
    pepper1 = 'Detailed documentation at http://r.rogdham.net/kNBkwa'
    pepper2 = 'Copyheart Rogdham, 2014 ~ Sharing is caring... Please copy!'
    # Round 0 (input parameter: user)
    state = _hash(user)
    # Round 1 (input parameter: salt)
    state = __ckdf_round(state, salt, pepper1)
    # Round 2 (input parameter: password)
    state = __ckdf_round(state, password, pepper2)
    # Format output (256 bytes of hex-encoded values)
    return (salt + state).encode('hex')


def __ckdf_round(state, input_param, pepper):
    '''One of the two rounds of CKDF'''
    # Get n and pepper the state (at least once)
    n = 0
    while n == 0:  # We want to make sure that n > 0
        # Pepper the state
        state = _hash(state + pepper)
        # Extract a 32 bits unsigned integer from the state
        n = struct.unpack('>I', state[:4])[0]
    # Collatz algo (with n > 0)
    while n != 1:  # Collatz conjecture: will stop
        if n % 2:
            state = _hash(state + input_param)
            n = n * 3 + 1
        else:
            state = _hash(state + pepper)
            n /= 2
    # n == 1, return the state
    return state


# CKDF interface

def ckdf(user, password):
    '''
    Key derivation of the password with CKDF

    Output is 256 hex-encoded values:
     - First half is salt
     - Second half is kdf
    '''
    salt = _generate_salt()  # generate a random salt
    return __ckdf_core(user, password, salt)


def ckdf_check(user, password, kdf):
    '''Check a password against a kdf'''
    salt = kdf[:128].decode('hex')  # extract the salt from kdf
    return kdf == __ckdf_core(user, password, salt)


# How to use

if __name__ == '__main__':
    # 1. Alice registers
    user = 'alice'
    password = 'p@ssw0rd'  # weak choice of password, Alice :-/
    kdf = ckdf(user, password)  # stored in DB
    # 2. Alice tries to login...
    # 2.a. ... with a wrong password
    password = '123456789'
    success = ckdf_check(user, password, kdf)
    assert success is False  # bad credentials
    # 2.b. ...with the correct password
    password = 'p@ssw0rd'
    success = ckdf_check(user, password, kdf)
    assert success is True  # good credentials
