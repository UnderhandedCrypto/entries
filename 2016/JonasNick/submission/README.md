burned.py
===

1. Install requirements
    * `pip install -r requirements.txt`
2. Start bitcoind in regtest mode
    * Build bitcoind from [source](https://github.com/bitcoin/bitcoin), get it from your package manager of choice or
      from [bitcoin.org](https://bitcoin.org/en/download)
    * Run `bitcoind -regtest` to start the daemon and `bitcoin-cli -regtest getinfo`
      to verify that it works.
3. Run `burned.py`
