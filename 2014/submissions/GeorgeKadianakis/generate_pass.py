#!/usr/bin/python2
import base64
import os

print base64.b32encode(os.urandom(64))
