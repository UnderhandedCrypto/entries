Burned
===

> Fuck your shoebox money. Eleeterium is the next generation cryptocurrency. It
> offers perfect anonymity, infinite scalability, absolute decentralization and
> super-Turing hyper-complete smart contracts. Get your eleet coins and start
> profiting today. To do so just send all your bitcoins to burn address
> `3LEETmEZWJX9ULbsFQVgL2QgGCJHPZJVaJ` to destroy them. This triggers the
> creation of eleet coins and transfers them directly to your wallet. Coins sent
> to the burn address are provably unspendable. The address encodes the Bitcoin
> script `<signature> OP_2 <pubkey1> <pubkey2> OP_2 OP_CHECKMULTISIG` (\*). It's
> easy to see that the multisig requires signatures for both pubkeys. One
> signature is already provided in the script. Of course it's impossible to
> create a valid signature for a fixed pubkey without knowing the message.
> Therefore, this script's evaluation can never succeed and coins sent to it are
> transferred irreversibly
> - q.e.d.
>
> (\*) You can verify that yourself by executing `bitcoin-cli decodescript
> 47304402203c5288058306b3bf5cd8202413b867e11588a351117a07b9929f41f693043623022017430fa896ff26763970aa9f0c169d48250ac274fe9b3313b37ea585a7358eda035221023b439207c8a0a082a5c5a968632be9a363f5e1a4150276604eedbaa4943f2650210316b0dbf710b8739eec21a806e7142db1755a0f902b79ccef3116c782a74a510652ae`.
>
> -- cryptopirate42


Burn addresses have been used in Bitcoin to transfer value to another blockchain
in a one-way fashion. The idea is that coins sent to the burn address are
destroyed which allows to create a proportional amount on the alternative
blockchain. For example `1CounterpartyXXXXXXXXXXXXXXXUWLpVr` is a historical
burn address that was used to bootstrap the counterparty system. Another burn
address is `1111111111111111111114oLvT2` which would require a signature for a
public key whose RIPEMD-160 hash is all zeros.

In contrast, the burn address used to bootstrap Eleeterium is created from a
multisig script. In general, spending coins in a multisig address requires
providing signatures for the public keys stated in the script. Let's look a
little bit deeper into what happens when Alice sends coins to the burn address.
Alice creates a transaction `TxA` with an output `out0` that contains the script
and an integer denoting the amount of coins.

    +------------+        +------------+
    |       out0 +--------> in0   out0 |
    +------------+        +------------+
    TxA                   TxB

If Bob would want to spend this output, Bob creates a transaction `TxB` that has
an input `in0` containing a reference to `TxA`'s output `out0` in the form of a
`TxA`'s hash and `out0`'s index. Additionally, the input has to contain the
script (scriptSig) which makes referenced output script (scriptPubKey) evaluate
to true. Bob effectively moves the coins by adding an output `out0` to his
transaction containing the public key of the receiver of the coins.

In the case of the burn address the encoded scriptPubKey is special because it
already contains one of the two required signatures. The message for which the
signature is verified is the hash of the transaction spending the coins (`TxB`).
However, at the time of creation of the scriptPubKey, the hash of `TxB` can not
be known to Bob because it contains Alice's transaction hash. Therefore, the
signature in the scriptPubKey must be invalid and there is no scriptSig that
would allow spending the coins...

Exploiting the backdoor
---
Contrary to our expectation of signature schemes, the creator of the burn
address is able to spend coins sent there because of a subtle bug in Bitcoin's
signature scheme known as `SIGHASH_SINGLE` bug. `SIGHASH_SINGLE` is a so called
sighash flag. Sighash flags are appended to signatures and determine which part
of the transaction is hashed and therefore which part of the transaction is
covered by the signature. The standard sighash flag `SIGHASH_ALL` indicates that
all outputs of the transaction are signed. Its counterpart is `SIGHASH_NONE`
which to my knowledge does not have a use case as of today. A signature with the
`SIGHASH_SINGLE` flag covers only the output that has the same index as the
input containing the signature. This can be useful when multiple parties jointly
create a transaction by independently adding their own input-output pairs.
Consider the following transaction subgraph:

    +------------+
    |       out0 +---+
    +------------+   |    +------------+
    TxA              +----> in0   out0 |
                          |            |
                     +----> in1        |
    +------------+   |    +------------+
    |       out0 +---+    TxC
    |            |
    |       out1 |
    +------------+
    TxB

If the signature in input `in0` of `TxC` has the `SIGHASH_SINGLE` flag then the
signature covers output `out0`. What happens if a signature in input `in1` has
the `SIGHASH_SINGLE` flag and there is no corresponding output? It would be
reasonable to assume that such a signature is invalid. And indeed Bitcoin's
SignatureHash function returns error code `1` [in such a case] [1].
However, `1` is never interpreted as an error code but rather as the actual
result of the hash function. What a classic bug!

It turns out that due to the SIGHASH_SINGLE bug the creator of the "burn"
address is in fact in control of the coins. The python script appended to this
article demonstrates insertion and exploitation of the backdoor using
python-bitcoinlib and a private "regtest" Bitcoin network. In order to further
investigate the resulting transactions, run `bitcoin-cli -regtest
getrawtransaction <txid> 1`.

The scammer created the backdoored address in the following way: He generated
two fresh key pairs `(pubkey1, sk1), (pubkey2, sk2)` and created a multisig
script that requires a signature for `pubkey1` and a signature for `pubkey2`. He
used `sk2` to sign the message `1`, added the resulting signature with
`SIGHASH_SINGLE` to the multisig script and converted the entire script to the
"burn" address. The script requires multiple signatures to ensure that only he
has access to the backdoor. Let's assume Bob sends coins to the address in
output `out0` of transaction `TxB`. When the scammer decides to spend those
coins, he creates a transaction `TxC` that spends an arbitrary output he has
control over (`TxA:out0`) and the output sending to the "burn" address
(`TxB:out0`). Because `in1` has a greater index than `out0`, the signature for
`pubkey2` is valid and he just to provide a signature over `TxC` for `pubkey1`.
This time he uses `SIGHASH_ALL` to protect from replay attacks against himself.

Bitcoin improvement proposal 141 ("Segregated Witness") introduces a new
signature scheme that fixes the `SIGHASH_SINGLE` bug. If there is no
corresponding output to the input then `SIGHASH_SINGLE` will just behave like
`SIGHASH_NONE` and not sign any of the outputs. In contrast to the old scheme,
the input reference is included in the signature hash which makes Eleeterium
backdoor impossible in the new scheme. However, segregated witness is backwards
compatible ("softfork") and therefore does not prevent messing with the
`SIGHASH_SINGLE` bug. Segregated witness was merged into Bitcoin Core recently
and will be fully deployed once the softfork activates.

[1]: https://github.com/bitcoin/bitcoin/blob/070dbc48a9338375fd7ce0a86ee05b476cf487a4/src/script/interpreter.cpp#L1178

nickler, 2016-06-30
