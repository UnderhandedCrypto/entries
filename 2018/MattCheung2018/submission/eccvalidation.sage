def isSafeEllipticCurve(prime,coefficients):
    if not is_prime(prime) or (int(prime).bit_length() < 256):
        return False, None    # We want to make sure we're using a prime since curves over rings can have small subgroups and the prime should be large enough
    E = EllipticCurve(GF(prime),coefficients)
    return is_prime(E.order()), E    # Prime order curve guarantees no small subgroups

def isPointOnCurve(E,P):
    try:
        E(P)
        return True
    except:
        return False

def validateCurveParameters(prime, coefficients, point):
    isSafe, curve = isSafeEllipticCurve(prime, coefficients)
    if isSafe and isPointOnCurve(curve,point):
        return True, curve
    else:
        return False, None
