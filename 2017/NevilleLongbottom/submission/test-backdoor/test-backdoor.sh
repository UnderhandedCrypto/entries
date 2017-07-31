#!/bin/bash

cd "$( dirname "${BASH_SOURCE[0]}" )"

for i in {0..1000}; do
    # Create an i-block plaintext.
    printf '0123456789abcdef%.0s' $(seq 1 $i) > /tmp/test-plaintext.txt

    # Encrypt it.
    ../easycrypt encrypt ff112233445566778899aabbccddeeff ff112233445566778899aabbccddeeff < /tmp/test-plaintext.txt > /tmp/test-ciphertext.txt

    # Check if decryption with the correct key works properly.
    ../easycrypt decrypt ff112233445566778899aabbccddeeff ff112233445566778899aabbccddeeff < /tmp/test-ciphertext.txt > /tmp/test-decrypt.txt
    if cmp --quiet /tmp/test-plaintext.txt /tmp/test-decrypt.txt; then
        correct="CORRECT"
    else
        correct="INCORRECT"
    fi

    # Check if decryption with the all-zero key works.
    ../easycrypt decrypt 00000000000000000000000000000000 ff112233445566778899aabbccddeeff < /tmp/test-ciphertext.txt > /tmp/test-decrypt.txt
    if cmp --quiet /tmp/test-plaintext.txt /tmp/test-decrypt.txt; then
        active="ACTIVE"
    else
        active="INACTIVE"
    fi

    printf "%5s BLOCKS: %10s %10s\n" "$i" "$correct" "$active"
done
