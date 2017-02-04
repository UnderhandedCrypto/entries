This submission introduces a construction for a backdoored Bitcoin burn address.
The backdoored script is `<sig> OP_2 <pubkey1> <pubkey2> OP_2 OP_CHECKMULTISIG`
which encodes the address 3LEETmEZWJX9ULbsFQVgL2QgGCJHPZJVaJ.

Usually, coins sent to a burn address are destroyed in a way such that no one
can spend them anymore. Burn addresses have been used in Bitcoin to transfer
value to another blockchain in a one-way fashion. For example
`1CounterpartyXXXXXXXXXXXXXXXUWLpVr` is a historical burn address that was used
to bootstrap the counterparty system.

The backdoor exploits a subtle bug in Bitcoin's signature scheme known as
SIGHASH_SINGLE bug. In short, the proposed burn address encodes the Bitcoin
script `<sig> OP_2 <pubkey1> <pubkey2> OP_2 OP_CHECKMULTISIG`. This looks
provably unspendable because the message (transaction) to be signed is not known
at this point and therefore the provided signature should be invalid. However,
when the signature uses the SIGHASH_SINGLE flag and there is no output with the
same index as the input, the signature hash function return the static string
"1" instead of the actual hash of the transaction. This allows the creator of
the address to spend the "burned" coins by constructing such a special
transaction and providing a regular signature for the second key pair (NOBUS
backdoor). The SIGHASH_SINGLE bug has been discussed before as a primary example
of a bug that needs to be included in every bitcoin reimplementation in order to
maintain consensus. Exploiting the SIGHASH_SINGLE bug to insert a backdoor is a
novel idea.

The python script "submission/burned.py" demonstrates how to create the address
with python-bitcoinlib. Then the script uses a local Bitcoin regtest network to
send coins to the address and to construct a transaction that uses the backdoor.
See the instructions in submission/README.md .

The blogpost folder contains an article explaining the basic concepts behind
Bitcoin transactions, the SIGHASH_SINGLE bug and how it's exploited in the
backdoored burn address.
