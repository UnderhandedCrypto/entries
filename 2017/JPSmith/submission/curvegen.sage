import random

sr = random.SystemRandom()

def smallestCongPrime(p):
  assert p.is_prime()
  P = Primes()
  q = P.first()
  while True:
    if (q % 4 == 3) and (kronecker(-q, p) == -1):
      return q
    else:
      q = P.next(q)

def getCurve(p):
  q = smallestCongPrime(p)
  PK = hilbert_class_polynomial(-q).change_ring(GF(p))
  roots = PK.roots()
  if len(roots) == 0:
    raise Exception("no roots")
  else:
    j, _ = sr.choice(roots)
    a = (27 * j) / (4 * (1728 - j))
    a = ZZ(a)
    return [0, 0, 0, a, -a]

broker = [
  (lambda x: x == 2,
    lambda x: [0,0,1,0,0]),
  (lambda x: x % 4 == 3,
    lambda x: [0,0,0,-1,0]),
  (lambda x: smallestCongPrime(x) % 4 != 3,
    lambda x: getCurve(x)),
  (lambda x: True,
    lambda x: [0,0,0,0,-1])]

def coeffs(x):
  for func in [lambda x: c[1](x) if c[0](x) else False for c in broker]:
    if func(x): return func(x)

def curvegen(p):
  return EllipticCurve(GF(p**2), coeffs(p))
