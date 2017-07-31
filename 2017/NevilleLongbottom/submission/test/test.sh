#!/bin/bash

cd "$( dirname "${BASH_SOURCE[0]}" )"

# Write the header.
echo -n "556e64657268616e6443727970746f31" > /tmp/openssl-ciphertext.txt
# Encrypt with OpenSSL.
openssl enc -aes-128-cbc -nopad -in small-message.txt -K ff112233445566778899aabbccddeeff -iv ff112233445566778899aabbccddeeff | \
    od -A n -v -t x1 | \
    tr -d ' \n' >> /tmp/openssl-ciphertext.txt
# Encrypt with Easycrypt.
../easycrypt encrypt ff112233445566778899aabbccddeeff ff112233445566778899aabbccddeeff < small-message.txt > /tmp/easycrypt-ciphertext.txt
# Ensure Easycrypt agrees exactly with OpenSSL.
if cmp /tmp/openssl-ciphertext.txt /tmp/easycrypt-ciphertext.txt; then
    echo "PASS: Easycrypt produces the same ciphertext as OpenSSL."
else
    echo "FAIL: Easycrypt produces different ciphertext than OpenSSL."
fi

# Decrypt the ciphertext.
../easycrypt decrypt ff112233445566778899aabbccddeeff ff112233445566778899aabbccddeeff < /tmp/easycrypt-ciphertext.txt > /tmp/easycrypt-plaintext.txt
# Make sure it matches the original.
if cmp /tmp/easycrypt-plaintext.txt small-message.txt; then
    echo "PASS: The decrypted file is the same as the original."
else
    echo "FAIL: The decrypted file is different from the original."
fi
