# Code based on sections 3 and 4 of http://www.monnerat.info/publications/anomalous.pdf
jD = {11: -(1<<15),
      19: -(1<<15)*(3**3),
      43: -(1<<18)*(3**3)*(5**3),
      67: -(1<<15)*(3**3)*(5**3)*(11**3),
      163: -(1<<18)*(3**3)*(5**3)*(23**3)*(29**3)}

def generateCurve(bitsize):
    while True:
        m = randint(1<<((bitsize>>1)-2),1<<((bitsize>>1)-1))
        for params in [(11,3), (19,5), (43,11), (67,17), (163,41)]:
            p = params[0]*m*(m+1)+params[1]
            if is_prime(p):
                k = GF(p)
                E = EllipticCurve(k,j=jD[params[0]])
                if E.order() != p:
                    E = E.quadratic_twist()
                return E, p
