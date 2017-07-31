DEFEATING THE NSA FOR DUMMIES
=============================

By japesinator + incertia

Algorithm
---------

As everyone knows, the NSA has general quantum computers which they use to look at our dickpics.
As such, we're all moving to post-quantum algorithms like SIDH, which are much harder for the NSA to crack!
However, the NSA is known to backdoor important curves, and as such choosing just one curve to use with this algo is obviously foolhardy compared to just generating a fresh one per use.
As Broker[1] showed, if the generalized Riemann hypothesis is true (it is, we just didn't include a proof b/c of these damn tiny margins), this can be done in O(log(q)^3), where q is the size of the field.
As that algorithm is mildly involved, we have provided a reference implementation in sage, to enable better NSA-busting for all!

Thus, we suggest the following algorithm for parameter negotiation:

  1. Alice picks some `p`, a prime, to send to Bob

  2. Bob runs `curvegen` from `curvegen.sage` and sends Alice the returned curve

  3. Alice chooses `P_A`, `Q_A`, Bob chooses `P_B`, `Q_B`, and we're off to the races!

Exploit
-------

Because of python/sage's late-binding, every entry in `broker` will be `(lambda x: True, lambda x: [0,0,0,0,-1])`.
In about half of all cases, this will work fine, and the curve will be supersingular.
However, about half the time, (more or less depending on how Alice picks `p`), it will not be supersingular, and while the algorithm will retain strength against classical attacks, it will be weak to any quantum ones.
We posit this is the first exclusively post-quantum backdoor and claim bragging rights.


[1] https://pdfs.semanticscholar.org/d514/98cea0335b3ff913793c521b840f79f22fad.pdf
