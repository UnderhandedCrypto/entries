Explain to the judges what your entry does
----

The submission consists of a backdoored key agreement scheme. There are two main parts to the submission:

- A simple noisy key agreement scheme
- A technique for creating a backdoor for the key agreement scheme 

The key agreement scheme
---
The key agreement scheme uses noisy modular multiplication, and is similar to LWE in 1 dimension. A private key is a random integer, while a public key is a noisy multiplication of the private key with a known base point `e` modulo a known parameter `n`:

    private_key_1 = x
    public_key_1 = ex + r mod n
    private_key_2 = y
    public_key_2 = ey + z mod n
    
    shared_secret = msb(x(ey + z)) == msb(y(ex + r))
                    msb(exy + xz) == msb(exy + yr)

The backdoor
----                    
The backdoor consists of the parameters `e` and `n`. The values `e` and `n` are actually a public key from a public key encryption scheme built using two techniques. 

The first technique is the use of a small modular multiplicative inverse, which enables the ability to invert the noisy multiplication function. Let `d` denote the inverse of `e`. Choose a small `d` such that `dr < n` and `x < d`. Then

    d(ex + r) mod n == x + dr
    
Since `x + dr < n` and `x < d`:
    
    x = x + dr mod d

The second technique is a noisy modulus, which protects the private key `d` from being computed from the public key `e`. In order to prevent the computation of `d` from `e`, the value of `e` is computed modulo `n - k` instead of modulo `n`, where `n` is the modulus that is used during the public key operations, and `n - k` is used during the private key operations. 

Performing modular reduction with approximately the same modulus yields approximately the same result. The magnitude of the difference between the two results is determined by `(a/n) * k`, where `a` is the value being reduced mod `n - k`. The sizes of the parameters are chosen such that the resultant difference is the same size as the noise value `r` used in the key agreement scheme.


Why it’s hard to detect
----
Computing the modular inverse of `e mod n` will yield some inverse that is not small and is therefore not usable to invert the noisy multiplication, nor detect the backdoor.

Computing the correct value of `d` from `e, n` is the same problem as computing the private exponent `d` in RSA when given `e, N`:

    totient(N) = (p - 1)(q - 1)
               = pq - p - q + 1
               = N - p - q + 1
               = N - k
             d = inverse(e, N - k)

Since RSA uses noisy modular inversion to compute the private decryption exponent, given an oracle that computes `d` from `e, n`, one could apply the same oracle to RSA to recover `d` from `e, N`. 
             
With RSA, recovering `d` when given `e, N` implies the ability to factor `N`. Proof of this statement can be found elsewhere, e.g. in the [original RSA paper](http://people.csail.mit.edu/rivest/Rsapaper.pdf) (see page 12). 

Therefore, the ability to recover the backdoor `d, n - k` from `e, n` implies the ability to break RSA and factor the modulus. This implication requires that `d, k` are chosen to be of appropriate size to mirror the structure of RSA e.g. `log(k) == log(p - q + 1)`, `d` should not be *too* small or it becomes vulnerable to Weiner's attack, etc.


Why it is a valuable entry that deserves to win 
----

It demonstrates interesting math techniques and uses them to create an undetectable backdoor. More advanced public-key encryption schemes can be constructed with the techniques, but they are beyond the scope of this submission.