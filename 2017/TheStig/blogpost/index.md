
*The rules allow to "include a blog-post-style explanation" with the
submission. I'm not sure what counts as blog-post-style, but since the more
technical explanation in [README-JUDGES.txt](../README-JUDGES.html) is a bit on
the technically heavy side, let's put the very same tools into the context of a
war story! :)*

The kidnapping of Comrade #852
------------------------------

Redland and Blueland has always been regional rivals. Now Redland has kidnapped
Blueland's lead scientist, and is forcing him to work for *them*.

Cast:
-----

- Comrade #852 - A former Blueland scientist, abducted by Redland from Blueland
- Comrade #1 - Dear Leader of the glorious state of Redland
- Comrade #23 - Redland's zealously faithful software developer


Day 8 of abduction
------------------

*Situation room of Redland.*

Comrade #23: "Dear Comrade #1, our magnificent leader. The scientist we
abducted from Blueland, who we now call Comrade #852, has finally agreed to
work for the glory of our great nation, Redland. His first suggestion is a
new steganography algorithm, something completely different from previous
'least significant bit in image or audio', 'number of spaces and tabs in text',
'adding whitespace', etc ones."

Comrade #1: "What is his new steganography based on?"

Comrade #23: "It is based on the ordering of lines in otherwise completely
inconspicuous lists. Files like lists of inventories, lists of IP addresses,
lists of sales transactions, lists of customers, lists of performance data,
etc. Anything that can happen to be unordered normally."

Comrade #1: "How does it encode the bits?"

Comrade #23: "Very simple: If by using string comparison a line compares
"bigger" than the previous one, that means a "1" bit. If the line is "smaller"
than the previous one, that means a "0" bit."

Comrade #1: "And how can the list be transmitted?"

Comrade #23: "That's the best part! The list can be transmitted in any way:
text file, excel file, printed out, even spoken on the phone or radio."

Comrade #1: "Sounds good. Go forward with it."

Comrade #23: "Thank you, our towering leader. I will code the encryption
myself, so we can know that it's implemented well, for the greater glory of our
brilliant nation, Redland!"

**Related code:**

[encoder1.go](../submission/1-encoder1/encoder1.go) - Comrade #23's naive encoder

[decoder.go](../submission/0-decoder/decoder.go) - Comrade #23's decoder


Day 64 of abduction
-------------------

*Situation room of Redland. Guards bring in Comrade #852, throw him on the floor.*

Comrade #1: "Blueland is somehow identifying the secret messages hidden with
your method, Comrade #852. You will now be executed."

Comrade #852: "Wait, the algorithm is not at fault, excellent Comrade #1, only
Comrade #23's implementation of it! *He* must be a traitor. Let *me*
reimplement the algorithm, for the greater glory of my new home Redland!"

Comrade #1: "OK, You get one more chance, Comrade #852. Fix the implementation.
If further messages are not intercepted, you get to live."

Comrade #852: "You are the wisest, Comrade #1."

**Related code:**

[analyze.go](../submission/2-analyze/analyze.go) - Blueland's statistical analysis tool

[encoder2.go](../submission/3-encoder2/encoder2.go) - Comrade #852's less predictable encoder that avoids analysis


Day 192 of abduction
--------------------

*Situation room of Redland. Comrade #852, well dressed, now clearly a respected
member of his new home country, on audience with Comrade #1.*

Comrade #852: "Oh, sumptuous leader of ours, Comrade #1. As you know, with my
new implementation of the algorithm, spiteful Blueland's statistical analysis
is not working anymore. Now I humbly ask for one thing: my new lover,
Comradess #278, is prisoner of war in abominable Blueland, I wish to send
some covert messages to her, saying how I miss her."

Comrade #1: "You serve me well, Comrade #852. I allow you to do that. But be
forewarned, all your messages will be screened, and we know your stego
algorithm!"

Comrade #852: "You are a shining light for all of us, our towering leader,
Comrade #1."

**Related code:**

[superencoder.go](../submission/4-superencoder/superencoder.go) - Comrade #852 version of the encoder, embedding a secondary "meta-message"


Day 256 of abduction - Day 1 of freedom
---------------------------------------

*The smoking ruins of the fortified gate of the compund.*

Comrade #1: "HOW DID THIS HAPPEN!? How could Blueland's commando so precisely extract Comrade #852!?"

Comrade #23: "We... We don't know... It's as if he somehow was able to
communicate with Blueland... Which is impossible! We were watching him all the
time! Reading his messages! Decoding his encoded ones! Could he... Could he be
using a **hidden channel INSIDE the hidden channel**?"

**Related code:**

[superdecoder.go](../submission/4-superencoder/superdecoder.go) - Blueland's decoder for the sencondary "meta-message" embedded into the stego's workings

