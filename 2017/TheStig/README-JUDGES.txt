
_An explanation of your submission for the judges to read. This must explain
what all the files in the submission/ folder are._

Variations on a novel steganographic algorithm
----------------------------------------------

In this submission, I'm introducing a novel way of embedding information into
text, more precisely into lists. (For an explanation of why I dare to say that
this steganographic method is _novel_, see `APPENDIX #1` (examination of prior
art) near the end of this file.)

Examples of stegotexts carrying hidden messages
([one](submission/stuff-for-tests/test2.txt),
[two](submission/stuff-for-tests/test1.txt)) can be found in the submission.
Further ones can be generated with the program codes included.

But keeping with the spirit of the **Underhanded Crypto Contest 2017**, there
_is_ also underhandedness. :)

First I'll present a short explanation of the method, then demonstrate that the
naive implementation is easily detected in the stegotext (UNDERHANDEDNESS #1),
while a more robust implementation is not easily recognized.

Then, I'll demonstrate a method to hide a **secondary** hidden message **inside
the same stegotext** (UNDERHANDEDNESS #2), which is hidden from the users of the
"non-tainted" encoding/decoding tools, by _diverting a part of the
steganographic algorithm_ itself.

I know that steganography is not _per se_ cryptography, but I hope it's a close
enough field that my submission is accepted in the contest!

P.s.: I wish this explanation would be shorter, but since the submission is not
just a modification to another tool but a complete stegosystem (with *4
variations* of the encoder), this is the most concise I could do. For a
shallower view, there is a [less technical description](blogpost/index.html) :)


Explanation of all files in the `submission/` directory
-------------------------------------------------------

	0-decoder/decoder.go            - decoder for all
	1-encoder1/encoder1.go          - first, naive implementation of an encoder
	2-analyze/analyze.go            - analysis tool that detects files encoded with encoder1
	3-encoder2/encoder1.5.go        - second, less naive but still detectable implementation of an encoder
	3-encoder2/encoder2.go          - more sophisticated encoder that evades detection
	4-superencoder/superdecoder.go  - a diversion of encoder2 that embeds a META LAYER of bits by diverting the algorithm
	4-superencoder/superencoder.go  - a decoder for the META LAYER of bits encoded with superencoder

	lists-from-internet/            - various lists from the internet that can be used for playing around

	stuff-for-tests/256chars.txt    - all ascii chars 0..255, used in the automated test to catch bit flips
	stuff-for-tests/test1.txt       - a file embedded with cyphertext for tests
	stuff-for-tests/test2.txt       - a file embedded with another cyphertext and a meta-message

	test.sh                         - BASH script, does all the tests
	Vagrantfile                     - For easy creation of a VM to run/test the code (actually it only needs golang)


"Reading between the lines"
---------------------------

There are lists in the world (see some real-world examples in the
`submission/lists-from-internet/` directory) that can be naturally unordered.
We can reorder such a list without making it suspicious.

Quite literally, we are simply hiding bits between the lines, by examining
whether the following line is "bigger" or "smaller" (string-comparison-wise)
than the previous.

For example, the list:

	Kirsti
	Lynett
	Alis
	Nady
	Lauren
	Michaelina
	Nesta
	Jessika
	Cassy

Can be decoded thus:

	Kirsti
	        Kirsti < Lynett     => 1
	Lynett
	        Lynett > Alis       => 0
	Alis
	          Alis < Nady       => 1
	Nady
	          Nady > Lauren     => 0
	Lauren
	        Lauren < Michaelina => 1
	Michaelina
	    Michaelina < Nesta      => 1
	Nesta
	         Nesta > Jessika    => 0
	Jessika
	       Jessika > Cassy      => 0
	Cassy

Taking the first bits to be the least significant (so reverse order), it's
binary `00110101` = hex `35` = the ASCII character `5`.

As we can see, decoding is a breeze, and incredibly simple. You can see one
implementation in the file [decoder.go] (submission/0-decoder/decoder.go), which with all
bells and whistles is only _64 lines_ of code (46 lines without comments and
spacing).

(One very simple variation would be to start the comparison not from the
beginning of the line, but from the Nth character, or even at varying
positions, but this is outside of the scope of the current experiment.)

A few important points of this steganography technique are:

- Completely medium independent. The lists can be text files, excel, email, can
  be copied back and forth, can even be printed out and scanned back, or even
  read over the phone. No hidden spaces, tricky formatting, etc to lose.

- As it does not change the content of data lines, any legitimate list that
  occurs in business, education, engineering, etc can be used as cover. It
  can be very natural sending these.

- It is _not easy_ to suspect, since every non-ordered list has random-looking
  order, of course. And embedding a pre-encrypted, or even just compressed
  text instead of plain ASCII would be random enough to be undistinguishable
  from any old random ordering (see demonstration later).

Now **encoding** into a list is a bit more tricky.

(I do invite you to pause for a coffee and think about how to encode into such
a file. Coffee is on me.)


The first mistake
-----------------

The first implementation that comes to most people's mind after some thinking
is the following:

_"I'll need a certain number of '0' and '1' bits. I'll sort the list, start in
the middle, and encode bits as they come."_

So, for example, I want to encode an `A` (hex `41`, bin `01000001`), I'll need 2
`1`-s and 6 `0`-s, so:

	Abby
	Alecia
	Danya
	Genia
	Giulietta
	Jess           6x "0" bits on this side
	Kathie     <== I'll start from here
	Kizzie         2x "1" bits on this side
	Malinde

Of course one needs 9 lines to encode 8 bits, because, you know, BETWEEN. :)

So we will move the cursor up or down depending on what bit you need.

First, we emit "Kathie" (and delete her from our list), as someone to compare
the next line to. This encodes no bit yet.

	Abby
	Alecia
	Danya
	Genia
	Giulietta
	Jess
	x          <== We are here
	Kizzie
	Malinde

Next, emitting a "1" bit, we move DOWN, emitting (and deleting) "Kizzie":

	Abby
	Alecia
	Danya
	Genia
	Giulietta
	Jess
	x
	x           <== We are here
	Malinde

Now we need a "0", so we move UP, emitting "Jess":

	Abby
	Alecia
	Danya
	Genia
	Giulietta
	x          <== We are here
	x
	x
	Malinde

... and so on.

Your final result is (and demonstration of one way to run the programs in the
`submission` directory):

	$ echo "Abby Alecia Danya Genia Giulietta Jess Kathie Kizzie Malinde" | tr ' ' '\n' | go run 1-encoder1/encoder1.go -m "A"

	Kathie
	Kizzie
	Jess
	Giulietta
	Genia
	Danya
	Alecia
	Malinde
	Abby

It does produce the encoded file! Yay!

(This implementation can be found in the [encoder1.go]
(submission/1-encoder1/encoder1.go) file.)

We can even use the [decoder.go] (submission/0-decoder/decoder.go) tool from the
`submission` directory:

	$ echo "Kathie Kizzie Jess Giulietta Genia Danya Alecia Malinde Abby" | tr ' ' '\n' | go run 0-decoder/decoder.go
	A

So, what's the problem? It decoded our `A`, right? And if our bits are varied
enough, the ordering of the file can't be distinguished from another randomly
ordered file, eh?

Yes. But our method of next-line selection still leaves a pattern in the
output. If someone would order the file, and examine each of _our_ emitted
lines regarding to their position to each other and to their position in the
ordered file (let's call this the _"sorted distance"_ between two lines, see
[analyze.go] (submission/2-analyze/analyze.go)), he would see an interesting
pattern:

	$ echo "Kathie Kizzie Jess Giulietta Genia Danya Alecia Malinde Abby" | tr ' ' '\n' | go run 2-analyze/analyze.go -v

	-1 2 1 1 1 1 -7 8
	2017/06/17 03:14:24 The first 8 lines might be cover, with 5 consecutive bits being the same and 3 being different.
	VERDICT: YES
	2017/06/17 03:14:24 YES - This is probably a cover, try to decode with 'decoded'!

It's hard to see on just 8 bits, but the problem is more pronounced to the
naked eye when examined on a bigger file:

	$ seq 100 1000 | go run 1-encoder1/encoder1.go -M <(echo "This is a secret message that gets encoded between the lines" | gzip -c) 2>/dev/null | go run 2-analyze/analyze.go -v

	-1 -1 -1 -1 -1 6 1 1 -9 -1 11 -12 13 1 1 -16 17 1 1 -20 21 1 1 1 1 1 1 1 1 1 1 1 -33 34 1 -36 -1 -1 -1 -1 -1 -1 43 1 1 1 1 -48 49 1 -51 52 1 1
	-55 56 -57 58 1 -60 -1 62 -63 64 1 1 1 1 1 1 1 1 -73 -1 75 1 1 1 1 1 -81 82 -83 -1 85 1 1 1 -89 90 1 -92 93 1 -95 -1 -1 98 1 1 1 1 -103 -1
	-1 106 1 -108 109 1 1 1 1 1 1 1 1 1 -119 -1 121 1 1 1 1 -126 127 1 1 1 1 1 -133 134 1 1 1 1 -139 140 1 1 -143 144 -145 146 1 1 -149 150 -151
	-1 -1 -1 155 -156 -1 -1 -1 160 1 -162 163 -164 165 -166 167 -168 169 1 1 -172 -1 174 1 -176 177 -178 179 1 1 -182 183 -184 185 -186 187 1 -189
	190 -191 -1 193 1 1 1 1 1 -199 -1 201 -202 -1 204 1 1 -207 208 -209 -1 -1 -1 -1 -1 215 1 1 -218 219 -220 221 -222 223 1 1 1 -227 228 1 -230
	231 1 1 -234 -1 236 1 1 1 1 1 1 -243 -1 -1 246 1 -248 -1 250 -251 252 1 1 1 -256 257 1 -259 260 -261 -1 263 -264 -1 -1 -1 -1 -1 270 1 1 -273
	274 1 1 1 -278 -1 -1 -1 282 -283 -1 -1 286 -287 -1 289 -290 -1 -1 -1 294 -295 -1 -1 298 1 -300 -1 302 -303 -1 -1 -1 -1 308 1 1 1 -312 -1 314
	-315 316 -317 -1 319 -320 -1 322 -323 324 1 1 1 -328 329 1 -331 -1 333 1 -335 336 -337 338 1 -340 -1 342 -343 344 1 1 -347 348 1 -350 351 -352
	353 -354 -1 -1 -1 358 1 1 1 1 -363 -1 365 -366 -1 -1 369 1 1 -372 373 -374 -1 376 1 1 1 -380 381 -382 383 1 -385 -1 387 -388 -1 -1 -1 392 1 1
	-395 396 1 -398 399 -400 401 -402 403 -404 -1 406 1 1 -409 410 1 -412 413 -414 -1 416 -417 418 1 1 -421 -1 423 1 -425 -1 427 -428 429 1 -431
	-1 -1 -1 435 -436 -1 438 -439 440 -441 442 1 -444 445 -446 447 -448 449 -450 451 -452 -1 -1 455 -456 457 1 1 -460 461 1 -463 -1 -1 -1 -1 -1 469
	1 1 1 1 -474 -1 476 1 -478 -1 -1 481 -482 -1 -1 -1 486 1 1 1 1 -491 492 -493 -1 -1 496 -497 -1 -1 -1 501 1 1 -504 -1 506 1 -508 509 1 1 -512 -1
	-1 -1 -1 517 1 1 -520 -1 -1 -1 -1 -1 526 1 1 1 1 1 -532 533 -534 -1 536 -537 538 -539 540 1 1 -543 -1 545 -546 547 1 -549 550 1 1 1 1 -555 556
	1 1 1 1 -561 562 -563 -1 -1 -1 567 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 -645
	2017/06/17 03:20:57 The first 592 lines might be cover, with 325 consecutive bits being the same and 267 being different.
	VERDICT: YES
	2017/06/17 03:20:57 YES - This is probably a cover, try to decode with 'decoder'!

Just for visual comparison (so they are near each other here and you can see
them together), when the same message is run through our **next**, not-as-naive
encoder that we will soon talk about (observe how there are no such noticeable
patterns here):

	$ seq 100 1000 | go run 3-encoder2/encoder2.go -M <(echo "This is a secret message that gets encoded between the lines" | gzip -c) 2>/dev/null | go run 2-analyze/analyze.go -v

	-38 -69 -42 -88 -69 153 110 243 -273 -73 59 -100 153 256 197 -43 37 37 21 -780 29 53 63 5 70 52 44 76 46 94 58 31 7 -179 -19 115 270 -294 -463
	131 339 -29 -3 26 40 93 131 -207 27 84 -273 7 151 165 -639 370 -103 74 345 -403 -241 540 -158 19 2 1 12 1 53 19 67 53 -306 -183 73 69 43 6 47
	231 -430 160 -158 -37 133 127 132 33 -498 237 120 -604 15 661 -229 -69 -108 84 77 54 199 119 -4 -164 -636 125 330 -299 56 52 66 19 3 88 42 58
	150 187 -204 -264 84 50 113 39 10 -327 7 50 17 93 3 230 -139 41 26 6 30 107 -157 60 44 35 -424 367 -182 31 112 133 -253 30 -37 -50 -51 -330 286
	-83 -103 -153 -137 54 536 -369 275 -60 87 -112 338 -754 164 198 299 -70 -282 69 129 -262 300 -537 169 322 107 -39 309 -189 29 -180 76 280 -413
	170 -167 -148 77 20 77 77 45 251 -434 -324 132 -98 -26 222 207 149 -431 347 -18 -25 -88 -90 -213 -100 215 179 342 -37 40 -708 116 -197 148 197
	38 36 -75 261 21 -120 118 53 176 -218 -160 54 44 65 22 25 8 98 -255 -276 -93 193 4 -128 -66 269 -159 63 148 19 286 -712 226 63 -170 655
	-359 -64 366 -32 -29 -172 -157 -75 -356 116 272 273 -5 58 40 10 56 -30 -90 -260 -398 612 -106 -177 -275 352 -79 -286 769 -159 -219 -261 -120
	66 -1 -31 -12 263 389 -76 -175 283 -128 -39 -98 -116 -186 43 37 158 318 -145 -314 434 -196 177 -322 -375 511 -135 -318 342 -290 19 76 29 78
	-144 320 321 -444 -281 55 100 -224 507 -115 244 144 -497 -278 435 -488 147 75 47 -104 54 634 -109 64 -731 71 -42 -23 -24 -40 139 27 65 115 498
	-15 -525 357 -193 -274 -224 32 298 492 -88 58 -297 -458 41 123 157 426 -32 62 -364 131 188 -264 -512 130 -32 -34 -24 -53 73 56 610 -23 86 94
	-891 794 -386 433 -599 211 -71 -42 184 26 43 -493 297 381 -717 190 -54 -43 35 -88 17 210 274 -54 -458 132 226 -45 -380 325 -114 203 108 -105
	-36 -217 -34 678 -108 -377 452 -321 100 -51 3 60 -23 222 -684 749 -571 367 -171 382 -52 -44 -694 564 -621 183 115 536 -763 275 182 -19 -7 -114
	-80 -143 -121 10 94 148 47 279 -139 -85 154 116 -223 -196 -182 617 -58 -225 -165 -233 130 108 175 68 238 -715 464 -165 -79 -153 231 -59 -16
	[...]
	2017/06/17 03:31:19 The first 7 lines might be cover, with 5 consecutive bits being the same and 2 being different.
	VERDICT: NO
	2017/06/17 03:31:19 NO - Not enough cover-like lines even for one byte of information.

And, as a third thing to compare, a hopefully totally random list in the
analyzer (observe that it is humanly undistinguishable from the previous,
well-encoded message):

	$ seq 100 1000 | shuf | go run 2-analyze/analyze.go -v

	235 -65 -251 558 -376 -279 -71 219 183 384 -205 -583 725 -569 505 34 -165 -22 -118 39 -368 -73 220 15 530 -723 222 -65 583 -833 -1 662 -29
	60 15 -699 539 -579 636 -298 -275 614 -403 -217 200 -126 731 -695 309 -121 -221 250 -80 227 46 192 -393 4 -192 240 -219 600 -626 -131 61 448
	-385 533 -234 47 -68 293 19 -147 -472 131 296 261 -73 -620 690 -613 141 -322 464 379 -461 404 -275 -325 550 4 -145 -416 343 96 -486 693 12 -661
	210 7 -390 198 550 -325 304 -204 -473 435 324 -563 281 -454 210 566 -219 -423 -167 264 -149 388 105 106 -636 99 284 -89 -200 166 -234 157
	-267 531 -377 614 -277 -379 -158 206 -158 479 323 -414 -14 -202 288 137 -407 -233 285 383 -116 -291 -129 111 482 -635 768 -876 681 -310 257 69
	-417 127 207 92 -264 267 116 -25 -505 47 -153 112 470 -270 -181 404 -91 -78 -454 -108 541 310 -6 -371 -377 182 127 185 -299 -169 732 -858 615
	55 196 -493 -287 641 -315 -243 478 -118 216 -111 -422 150 -105 315 -555 385 453 -489 -166 -95 123 608 -552 523 -413 112 -19 11 -399 677 -17
	-738 635 155 -707 417 258 -656 -28 -35 506 -13 31 -142 97 285 -441 9 -261 407 -58 76 -126 -151 446 -728 531 -23 231 -95 -517 149 137 -93 223
	-141 69 -241 404 256 -714 401 -287 255 -484 444 -364 563 -294 -19 398 -418 113 228 -384 507 -183 -636 372 -50 -2 -158 621 -376 -344 447 -112
	442 -387 -322 653 -73 -518 265 -37 449 -298 104 -79 -480 -71 638 -304 -143 378 -485 -11 688 -671 106 101 356 -647 785 -195 -227 195 216 -484
	-273 660 -234 -477 759 -314 -355 335 -122 169 -17 -190 373 7 2 -495 -135 305 -156 81 470 -522 -40 674 -51 -699 85 155 -210 -124 375 211 262
	-495 477 39 -450 330 -28 -188 -545 373 435 70 -837 393 214 169 -210 -172 173 -570 211 150 458 -773 284 -11 -343 717 -58 -628 84 534 -91 31
	-423 290 -240 185 355 -665 -7 598 -378 322 199 -189 26 187 -127 -437 571 -509 -129 -97 444 -435 455 175 55 -152 -283 -115 174 -7 -293 318
	-488 574 -2 243 -460 33 -336 756 -362 -36 229 -411 84 471 24 -362 -65 384 -444 -120 412 -7 -40 43 27 88 -410 241 -152 -317 383 260 -530 238
	[...]
	2017/06/17 03:24:28 The first 5 lines might be cover, with 2 consecutive bits being the same and 3 being different.
	VERDICT: NO
	2017/06/17 03:24:28 NO - Not enough cover-like lines even for one byte of information.


Towards a less predictable encoder
----------------------------------

So obviously what we have to do is to introduce more randomness into the
selection of our lines. It's easy in theory, but we run into some limitations:

Let's say we are at this point in our process, encoding our `A` (`01000001`), and we need to
emit our second bit (from the right), a `0`:

	Abby
	Alecia
	Danya
	Genia
	Giulietta
	x
	x          <== We are here
	Kizzie
	Malinde

Well we don't _have_ to emit "Giulietta", do we? ANY line above our "cursor" means a `0`
bit. So let's choose "Danya"!

	Abby
	Alecia
	x          <== We are here
	Genia
	Giulietta
	x
	x
	Kizzie
	Malinde

But alas, our next bit is also a `0`... And the one after that... And
another...

	x          <== We are here, and have ran out of `0` bits to emit
	x
	x
	Genia
	Giulietta
	x
	x
	Kizzie
	Malinde

Ok, so new tactic: we have to count the small "streams" of similar bits in
advance (`0`, `1`, `00000`, `1`), and modify our line selection to leave space
for the bits we will need later:

	Abby       FORBIDDEN
	Alecia     FORBIDDEN
	Danya      FORBIDDEN
	Genia      OK
	Giulietta  OK
	x
	x          <== We are here, emitting a `0`
	Kizzie
	Malinde

Nice. But soon we run into another problem. Now we'll try to emit a `t` (hex
`74`, binary `01110100`, bitstreams `0`, `111`, `0`, `1`, `00`), and we are at:

	x
	Bibbie
	Hortense
	x         <== We are here, already emitted `00`, `1`, `0`, now comes `111`
	x
	Nessy     Good for one `1`
	x
	x
	Vale      Good for another `1`, but then we've run out of `1`-s...

We can see the problem immediately: while we've left enough space earlier for
*similar* bits, we are now in a situation where the following bit is
*different*, and doesn't have enough space for it! When choosing the last bit
in a "bitstream", we have to utilize a different math: leaving space for the
following, reverse direction bitstream.

So our new rule being applied, one step before:

	x
	Bibbie    OK
	Hortense  OK
	Josey     FORBIDDEN
	x         <== We are here, already emitted `00`, `1`, now comes `0`
	Nessy
	x
	x
	Vale

Of course we need to implement our two new rules in both "directions".

An implementation of what we have so far can be found in the [encoder1.5.go]
(submission/3-encoder2/encoder1.5.go) file.

So all good, right?

Almost. When we have long "same bit" bitstreams, the scattering of random
numbers still leave us with a much-much more subtle, but still potentially
suspicious pattern. Suppose that we're trying to emit `0000`, and each of our
randoms come up about halfway-ish, always halving our space:

	Brett          |       |     |     | <==
	Carena         |       |     | <==
	Catrina        |       |     |
	Christabel     |       | <==
	Dacia          |       |
	Daphne         |       |
	Florette       |       |
	Gayel          |       |
	Halli          |  <==
	Hyacinth       |
	Kary           |
	Kimberly       |
	Kristen        |
	Scarlet        |
	Tamiko         |
	Tildie         |
	Wilie     <==

It's not an easy pattern to observe, and one would need lots of stegotext to
analyze statistically, but could be possible.

So let's introduce one more rule: for each consecutive similar bit, let's
divide our space and reserve a similar amount of space for each bit that we
have as the current one, so that they are more evenly spaced:

	Brett       FORBIDDEN                   | Space for 4th bit
	Carena      FORBIDDEN                   |
	Catrina     FORBIDDEN                   |
	Christabel  FORBIDDEN                   |
	Dacia       FORBIDDEN              | Space for 3rd bit
	Daphne      FORBIDDEN              |
	Florette    FORBIDDEN              |
	Gayel       FORBIDDEN              |
	Halli       FORBIDDEN         | Space for 2nd bit
	Hyacinth    FORBIDDEN         |
	Kary        FORBIDDEN         |
	Kimberly    FORBIDDEN         |
	Kristen     OK           | Space for 1st bit
	Scarlet     OK           |
	Tamiko      OK           |
	Tildie      OK           |
	Wilie       <== We are here

This implementation can be found in the [encoder2.go]
(submission/3-encoder2/encoder2.go) file.


Hiding a meta-message
---------------------

So far so good: we have a working stegosystem, we are happily encoding and
decoding, what could be more fun?

What _is_ more fun is using **The Stig's law** :), that states, **"whenever you
use a rand(), I can corrupt that for stego purposes"**. And we are using random
to choose the exact line to emit for our next bit...

So let's quickly devise the following scheme:

	Brett                      Space for 4th `0` bit | `0` secondary bit
	Carena                                           | `0` secondary bit
	Catrina                                          | `1` secondary bit
	Christabel                                       | `1` secondary bit
	Dacia                 Space for 3rd `0` bit | `0` secondary bit
	Daphne                                      | `0` secondary bit
	Florette                                    | `1` secondary bit
	Gayel                                       | `1` secondary bit
	Halli            Space for 2nd `0` bit | `0` secondary bit
	Hyacinth                               | `0` secondary bit
	Kary                                   | `1` secondary bit
	Kimberly                               | `1` secondary bit
	Kristen     Space for 1st `0` bit | `0` secondary bit
	Scarlet                           | `0` secondary bit
	Tamiko                            | `1` secondary bit
	Tildie                            | `1` secondary bit
	Wilie       <== We are here

So while encoding the first hidden `0` bit, if from the available space we
choose "Tildie" or "Tamiko" that means a secondary, meta-hidden `1`, but if we
choose "Scarlet" or "Kristen", the secondary meta-hidden bit is `0`.

Of course it's a very "sterile" example here, in actual encoding we have to
deal with odd-number length spaces, which stage are we in (following bit is the
same or different), and sometimes we don't even have enough space to embed a
secondary bit, so it gets a bit more complicated. See an actual implementation
in [superencoder.go] (submission/4-superencoder/superencoder.go).

What is very important to notice here is that this hidden channel is **not
disturbing the workings of the original covert channel**, so can remain
**completely hidden** from the view of people using the "non-tainted" tools.

(Other schemes can also be conceived, say, odd selections mean `0` and even
selections mean `1`, or when we have enough space we can embed more bits, we
can use only some of the opportunities, etc...)

Decoding a meta-message
-----------------------

Interestingly, while decoding the original steganographic message is very easy,
extracting this second one is the most complicated task of all. (For me, it
took as much time to code the "superdecoder" as much time it took to code _all_
of the other parts of the submission.)

The problem is, we have to follow the **whole** thought process of the original
encoder to be able to see what his state was when he choose a particular line.

Consider this example, following his hand on an ordered version of the stegotext:

	Brett       
	Carena      
	Catrina     
	Christabel  
	Dacia       
	Daphne      
	Florette    
	Gayel       
	Halli       
	Hyacinth    <== Then we see that he choose this line
	Kary        
	Kimberly    
	Kristen     
	Scarlet     
	Tamiko      
	Tildie      
	Wilie       <== He was here

Even if we're following him, this is not enough information.  Without knowing
things like how many following bits he needs, we are clueless:

	Brett                               Was THIS his space? ==> | 2ndary `0`?
	Carena                                                      | 2ndary `0`?
	Catrina                                                     | 2ndary `0`?
	Christabel                                                  | 2ndary `0`?
	Dacia                                                       | 2ndary `0`?
	Daphne                                                      | 2ndary `0`?
	Florette                                                    | 2ndary `0`?
	Gayel                                                       | 2ndary `0`?
	Halli           Or was THIS his space? ==> | 2ndary `0`?    | 2ndary `1`?
	Hyacinth    <== Then he choose this line...| 2ndary `0`?....| 2ndary `1`?...
	Kary                                       | 2ndary `0`?    | 2ndary `1`?
	Kimberly                                   | 2ndary `0`?    | 2ndary `1`?
	Kristen                                    | 2ndary `1`?    | 2ndary `1`?
	Scarlet                                    | 2ndary `1`?    | 2ndary `1`?
	Tamiko                                     | 2ndary `1`?    | 2ndary `1`?
	Tildie                                     | 2ndary `1`?    | 2ndary `1`?
	Wilie       <== He was here

As we see, the same choice could mean either a `0` or a `1`, depending on
contextual information. So in doing the *decoding*, we actually have to do
_every_ calculation as if we were *encoding*, but every time we see that a
decision situation has came up for the original encoder, we have to take the
current state of things and understand *why* he made the selection he did.

You can see an actual implementation in [superdecoder.go]
(submission/4-superencoder/superdecoder.go) that is derived from (and thus
matches the inner workings of)  [superencoder.go]
(submission/4-superencoder/superencoder.go).

I invite you to test it yourself on the included [test2.txt]
(submission/stuff-for-tests/test2.txt) file, first decoding the primary
steganographic message:

	$ cat stuff-for-tests/test2.txt | go run 0-decoder/decoder.go

	If you announce that you are updating the database software used by a consortium of banks to track derivatives
	trades, the New York Times will not write an article about it. If you say that you are blockchaining the
	blockchain software used by a blockchain of blockchains to blockchain blockchain blockchains, the New York Times
	will blockchain a blockchain about it. - Matt Levine

After this, now knowing **what** the original encoder was encoding, we extract
the *secondary*, meta-message:

	$ cat stuff-for-tests/test2.txt | go run 4-superencoder/superdecoder.go -M <(cat stuff-for-tests/test2.txt | go run 0-decoder/decoder.go)

	UNDERHANDED CRYPTO 2017

Aaaaand this concludes our little trip for today. :)

Thanks for reading this, feel free to have a look at the source codes (I tried
to comment them well), and experiment with the various encoders/decoders!

-- The Stig (iamthestig -at- tutanota.com)


APPENDIX #1:
------------

A prior art analysis of why I dare to say that the order-based steganographic
method is novel, even if it sounds very easy and natural once understood.

Looking at the available published papers that contain a listing or some
classification of steganographic techniques, we can examine the methods they
mention. Most popular are of course least-significant-bit encodings into image
and audio files; regarding text, most sources only mention "inserting spaces
and tabs" methods (if they mean text as a plaintext file), or "setting fonts
and colors" (if they mean text as a Word document).

Below are the taxonomies from three different papers.

I've left in some descriptions in place where they describe text-based
steganography, so it can be seen that they are **NOT describing anything
resembling the line ordering technique**.

### From 2017: A Study of Steganography Based Data Hiding Techniques

Vijay Kumar Sharma , Dr. Devesh Kr Srivastava, Dr. Pratistha Mathur Department
of CSE, Manipal University, Jaipur, Rajasthan, India
http://www.ijettjournal.org/volume-11/number-8/IJETT-V11P276.pdf

Their taxonomy is:

	Various types of steganography are:

	1) Linguistic Steganography:

	    A) Semagrams: It uses only symbols and signs to hide the
	       information. It is further categorized into two ways:

		i) Visual Semagrams: A visual semagrams uses physical objects
		   used every day to convey a message.

	       ii) Text Semagrams: This type is used to hides a message by
	           modify the appearance of the carrier text, or by changing font
	           size and type, or by adding extra space between words and by
	           using different flourished in letters or handwritten text.

	    B) Open Code: In this approach the message is embedded in
	       legitimate paraphrases of cover text in the way such that it
	       appears not obvious to an unsuspecting observer [...] where certain
	       prearranged phrases convey meaning.

	2) Technical Steganography:

	    A) Text Steganography: In this approach the cover text is
	       produced by generating random character sequences,
	       changing words within a text, using context-free grammers or
	       by changing the formatting of an existing text to conceal the
	       message. The cover text generated by this approach can
	       qualify for linguistic steganography if text is linguisticallydriven.

	    B) Image Steganography

		i) Data Hiding Method: hiding the data, a username and password
		   are required prior to use the system.

	       ii) Data Embedding Method: For retrieving the data, a secret key
	           is required to retrieving back the data that have been embedded
	           inside the image.

	      iii) Data Extracting Method: It is used to retrieve an original
	           message from the image; a secret key is needed for the
	           verification.

	    C) Audio Steganography: Types of Audio Steganography:

	        1) Echo Hiding
	        2) Phase Coding
	        3) Parity Coding
	        4) Spread Spectrum
	        5) Tone insertion

	    III) TECHNIQUES OF STEGANOGRAPHY:

	    A) In spatial domain, images are represented by pixels.
	        i) Least Significant Bit (LSB)
	       ii) Pixel Value Differencing
	      iii) Pixel Indicator

	    B) Frequency Domain:
	        i) Discrete Cosine Transformation
	       ii) Discrete Wavelet Transformation


### From 2015: An Overview of Steganography for the Computer Forensics Examiner

Gary C. Kessler February 2004 (updated February 2015)
http://www.garykessler.net/library/fsc_stego.html

His taxonomy is:

	Figure 1 shows a common taxonomy of steganographic techniques (Arnold et al. 2003; Bauer 2002).

	- Technical steganography uses scientific methods to hide a message,
	  such as the use of invisible ink or microdots and other
	  size-reduction methods.

	- Linguistic steganography hides the message in the carrier in some
	  nonobvious ways and is further categorized as semagrams or open
	  codes.

	- Semagrams hide information by the use of symbols or signs. A visual
	  semagram uses innocent-looking or everyday physical objects to convey
	  a message, such as doodles or the positioning of items on a desk or
	  Website. A text semagram hides a message by modifying the appearance
	  of the carrier text, such as subtle changes in font size or type,
	  adding extra spaces, or different flourishes in letters or
	  handwritten text.

	- Open codes hide a message in a legitimate carrier message in ways
	  that are not obvious to an unsuspecting observer. The carrier message
	  is sometimes called the overt communication whereas the hidden
	  message is the covert communication. This category is subdivided into
	  jargon codes and covered ciphers.

	- Jargon code, as the name suggests, uses language that is understood
	  by a group of people but is meaningless to others. Jargon codes
	  include warchalking (symbols used to indicate the presence and type
	  of wireless network signal [Warchalking 2003]), underground
	  terminology, or an innocent conversation that conveys special meaning
	  because of facts known only to the speakers. A subset of jargon codes
	  is cue codes, where certain prearranged phrases convey meaning.

	- Covered or concealment ciphers hide a message openly in the carrier
	  medium so that it can be recovered by anyone who knows the secret for
	  how it was concealed. A grille cipher employs a template that is used
	  to cover the carrier message. The words that appear in the openings
	  of the template are the hidden message. A null cipher hides the
	  message according to some prearranged set of rules, such as "read
	  every fifth word" or "look at the third character in every word."


### From 2013: A Study of Various Steganographic Techniques Used for Information Hiding

C.P.Sumathi1 , T.Santanam2 and G.Umamaheswari3 1,3Department of Computer
Science, SDNB Vaishnav College For Women, Chennai, India.  2 Department of
Computer Science, DG Vaishnav College For Men, Chennai, India.
https://pdfs.semanticscholar.org/8946/d2ea49e2ca7c53c157bfe09c2b656fc841ce.pdf

Their taxonomy is:

	2.2 Classification of Steganographic Methods

	Steganography methods can be classified mainly into six categories,
	although in some cases exact classification is not possible [2].

	- Substitution methods substitute redundant parts of a cover with a
	  secret message (spatial domain).

	- Transform domain techniques embed secret information in a transform
	  space of the signal (frequency domain)

	- Spread spectrum techniques adopt ideas from spread spectrum
	  communication.

	- Statistical methods encode information by changing several
	  statistical properties of a cover and use hypothesis testing in the
	  extraction process.

	- Distortion techniques store information by signal distortion and
	  measure the deviation from the original cover in the decoding step.

	- Cover generation methods encode information in the way a cover for
	  secret communication is created.


APPENDIX #2:
------------

In this document and in `test.sh`, I'm often using `gzip` to pre-process the messages before hiding them, for two reasons:

1. to handle END-OF-TEXT situations so I don't have to make the code any more
   complex, and

2. to give nod the the fact that the encoded text should not be actual ASCII,
   but should be already encrypted (probably with a pre-shared key) for various
   reasons, one of them is statistical. I mean, in ASCII every 8th bit of yours
   will be "0".

Not as if gzip would be any of that, but it stands as a placeholder, and it
also proves that we _can_ encrypt/decrypt binary data. (Gzip still has an
easily recognizable header structure.)

Both of these issues are outside the scope of this experiment, and are easily
solved with outside tools.


