# Underhanded Crypto contest submission
### by Ron Mordechai
<hr>

This entry falls into the backdoor category.  My entry consists of an RC4
implementation that looks fine, but actually contains a dangerous bug.  The
swap function used in the implementation does not check if the addresses it
receives are the same, and will zero-out their values if they are.  This means
That after a long enough stream of data, the algorithm will begin to spit out
plain bytes instead of encrypted ones.

In the code example I try to encrypt 100KB of 'x' bytes.  For small amounts of
data it seems to work, outputting pretty random bytes, but if we use a larger
stream of data we begin seeing more and more 'x's. 

From what I understand, this is a known bug in old amateur RC4 implementations,
so I'm not sure if it qualifies or not, but I think it's simple enough not be a
waste of anybody's time.
