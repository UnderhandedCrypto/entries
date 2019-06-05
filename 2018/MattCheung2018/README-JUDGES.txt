This submission involves code to perform Elliptic Curve parameter validation.  The idea is that this is something someone who knows some basic abstract algebra and known attacks that result from discrete log based systems using small subgroups.  The idea behind this validation code is to make sure that the order of the curve is prime so there are no small subgroups and any points given are on the curve.  This could be used in an ECDH or ECDSA scenario where public parameters can be negotiated on the fly.  All of the code is implemented in Sagemath to make implementation easier since it provides much of the Elliptic Curve functionality.  The key flaw is that only checking that the curve has prime order leads to validating curves vulnerable to the Smart attack.  This attack allows for computing discrete logs in linear time.  Included in the submission directory:
eccvalidation.sage
attack.sage
anomalous_curves.sage

The attack involves lifting the curve and the points to the p-adic field Q_p where p is the prime the field is over.  If the size of the curve is equal to the size of the field the curve is over and it's prime, this attack will work.  The attack was published in https://wstein.org/edu/2010/414/projects/novotney.pdf which I borrowed for the attack portion.  The file attack.sage was NOT written by me since the code was published in the paper.

To demonstrate this works, http://www.monnerat.info/publications/anomalous.pdf contains an algorithm for generating these types of curves which are called Anomalous curves.  In section 4 it shows that if a prime is constructed from a given smaller integer using one of a particular form, then there is a corresponding j-invariant we can use to construct a curve.  We can use the Sagemath EllipticCurve constructor by specifying j-invariant instead of coefficients to construct the curve we want.  The curve we want is either that curve or the quadratic twist, which Sagemath can also compute.  The code to generate such a curve can be found in anomalous_curves.sage which I wrote.  Below is an example:

p = 83700229114514385393037569582425911464071356756995347625759655498388899397653
E = EllipticCurve(GF(p), [83700229114514385393037569582425911464071356756995347625759655498385508302869, + 77986137112576])
p == E.cardinality()
P = E([81253531324030142815434207090257618756672185255045895692718672699303054061901, 31973562530767816272947970638867568867501178101098254683058864383596216534907])
Q = E([21925866658300736446486299192905185029582991726538527303221304067044999185043, 70590734704587114090756104919591401476169946288535393927501082281140835849402])

n = SmartAttack(P,Q,p,8)
Q == n*P

Running the above in Sagemath will run very quickly compute the discrete log of Q with respect to P.

I think this would be difficult for anyone to spot who doesn't know the intricacies of ECC, especially by someone who is more familiar with finite field discrete log based cryptography.  This was an attack that I only just learned about actively looking for known attacks on ECC.
