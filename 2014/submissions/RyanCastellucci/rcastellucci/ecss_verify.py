#!/usr/bin/env python
# coding=utf8

from ecss import *

ss = b64_to_hex(argv[1])
vr1s2 = decode_selfsig(ss)

Q = ecdsa_selfverify(ss)

print 'Q:  %s' % Q if Q else "VERIFICATION FAILED"
# vim: sw=4 ts=4 et ai si
