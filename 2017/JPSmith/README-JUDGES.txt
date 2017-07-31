This submission purportedly contains an implementation of Broker's[1] algorithm for efficiently generating supersingular curves for use in SIDH.
However, `curvegen.sage` is broken and will fail to return supersingular curves depending on the given `p`.
Because of python's late binding, every element in `broker` is equal to `(lambda x: True, lambda x: [0,0,0,0,-1])` and so `find` will always return `[0,0,0,0,-1]`.
An adversary with the ability to set `p` can then choose whether or not the curve used in SIDH is supersingular by choosing if `smallestCongPrime(p) % 4 == 3`.
Even if an adversary cannot control `p` though, the curve should not be supersingular around half the time.
Whether or not this is the case should have no effect on security against attacks on a non-quantum computer, but makes SIDH, a post-quantum algorithm, weak against quantum attacks.

[1] https://pdfs.semanticscholar.org/d514/98cea0335b3ff913793c521b840f79f22fad.pdf
