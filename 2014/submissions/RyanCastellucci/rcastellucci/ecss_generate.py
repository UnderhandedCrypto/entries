#!/usr/bin/env python
# coding=utf8

from ecss import *

ss, d, Q = ecdsa_selfsign(argv[1])
print "d:  %s" % d
print "Q:  %s" % Q
print "ss: %s" % hex_to_b64(ss)

# vim: sw=4 ts=4 et ai si
