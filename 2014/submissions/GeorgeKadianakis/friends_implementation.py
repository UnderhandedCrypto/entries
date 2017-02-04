#!/usr/bin/python2
# Unlicense

"""
This is some Python code that simulates Alice uploading a file to the server.

In the end, it shows how the server can derive the same encryption key
without ever learning the actual password.
"""

import hashlib, hmac

HASH_LEN = 32 # 256/8

###### Out-of-band preparation:

# Alice wants to securely send the following string to Bob through Friends!:
file_plaintext = "\x42" * HASH_LEN      # just a string with a bunch of 0x42

#    password = base64.b32encode(os.urandom(64))
# Alice and Bob have already exchanged 'password'. In this case, it's:
password = "Q3MR5FQJZJWGPE6UQCUK4S5UVKMZMAOUAAXRDNQ52TSBQ4R42S2BCJQKDC3PULDVA5BTRHUJZO2TPRNKR647JHMC27APOXB4PLU54SQ="

###### Protocol

# Alice derives the password hash 'h_password' and the encryption key 'k':

h_password = hashlib.sha256(password).digest()
# h_password = "0x1db67edab5141fdb3f431f013afcc231d509a03a7ce03e0a7bf7f0a426ca8e52"

k = hmac.new(password, "Friends!", digestmod=hashlib.sha256).hexdigest()
# k = "98b14113d324f437b017f30c52e40d8773af7845cc3f6413edf3b3a6047d1b7f"

# Now Alice encrypts the string:
#   file_ciphertext = bytewise_xor(file_plaintext, k)
# and sends it to the Friends! server along with h_password.

####### Attack!!!

# Since the Friends! server knows h_password, and HMAC hashes long
# keys (like our password), the server can just do:

k_mitm = hmac.new(h_password, "Friends!", digestmod=hashlib.sha256).hexdigest()
print "[*] Derived k_mitm = %s (k = %s)" % (k, k_mitm)
assert(k == k_mitm)
