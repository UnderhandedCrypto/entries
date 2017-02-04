#!/bin/sh

# generate a shedload of signatures
# cheat: detect and dump affected signatures in 'sigs'

GPG="../g10/gpg --homedir ./gnupg --no-default-keyring"

count=0
total=0

while true; do
  rm sig.asc
  echo hello world | $GPG --clearsign --armor --debug=4 --use-agent -o sig.asc 2> log
  total=$(($total + 1))
  echo "made $total sigs"

  if grep -F '.+' log; then
    mv sig.asc sigs/sig.$count.asc
    mv log sigs/log.$count.txt
    count=$(($count + 1))
    echo "found $count"
  fi
done
