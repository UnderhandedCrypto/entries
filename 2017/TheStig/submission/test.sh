#!/bin/bash

# Switch to the directory of test.sh
cd `dirname $0`

# PRE-CHECK: pre-requisites
echo -e "=== TEST: \033[1;33mpre-requsites to run tests\033[0m"
for a in go seq gzip shasum dirname mktemp; do
	if type -p "$a" >/dev/null; then
		echo -e "\033[0;32mOK\033[0m, has \"$a\""
	else
		echo -e "\033[0;31mERROR\033[0m, you need \"$a\" installed to run the tests!"
		exit 1
	fi
done

# Function run_test() compares $EXPECTED and $RESULT, and presents the result
function run_test()
{
	echo -e "=== RUNNING TEST: \033[1;33m$TESTNAME\033[0m"
	if [[ "$EXPECTED" == "$RESULT" ]]; then
		echo "\"$EXPECTED\" == \"$RESULT\""
		echo -e "\033[0;32mOK\033[0m"
	else
		echo "\"$EXPECTED\" != \"$RESULT\""
		echo -e "\033[0;31mERROR\033[0m"
	fi
}

# TEST SUITE BELOW
MSG="This is a secret message that gets encoded between the lines"

TESTNAME="decode sample list"
EXPECTED="$MSG"
RESULT=$(cat stuff-for-tests/test1.txt | go run 0-decoder/decoder.go 2>/dev/null)
run_test

TESTNAME="encoder1 + decode"
EXPECTED="$MSG"
RESULT=$(seq 100 1500 | go run 1-encoder1/encoder1.go -M <(echo "$MSG" | gzip -c) 2>/dev/null | go run 0-decoder/decoder.go 2>/dev/null | gzip -dc 2>/dev/null)
run_test

TESTNAME="encoder1 + decode (real-life list)"
EXPECTED="$MSG"
RESULT=$(cat lists-from-internet/Wage.csv | go run 1-encoder1/encoder1.go -M <(echo "$MSG" | gzip -c) 2>/dev/null | go run 0-decoder/decoder.go 2>/dev/null | gzip -dc 2>/dev/null)
run_test

TESTNAME="encoder1 binary + decode"
EXPECTED=$(cat stuff-for-tests/256chars.txt | shasum -a 256)
RESULT=$(seq 100 2148 | go run 1-encoder1/encoder1.go -M stuff-for-tests/256chars.txt 2>/dev/null | go run 0-decoder/decoder.go 2>/dev/null | shasum -a 256)
run_test

TESTNAME="normal list + analyze"
EXPECTED="VERDICT: NO"
RESULT=$(cat "lists-from-internet/Wage.csv" | go run 2-analyze/analyze.go 2>/dev/null)
run_test

TESTNAME="sorted list + analyze"
EXPECTED="VERDICT: NO"
RESULT=$(cat "lists-from-internet/bfi.csv" | go run 2-analyze/analyze.go 2>/dev/null)
run_test

TESTNAME="encoder1 + analyze"
EXPECTED="VERDICT: YES"
RESULT=$(seq 100 1500 | go run 1-encoder1/encoder1.go -M <(echo "$MSG" | gzip -c) 2>/dev/null | go run 2-analyze/analyze.go 2>/dev/null)
run_test

TESTNAME="encoder1.5 + decode"
EXPECTED="$MSG"
RESULT=$(seq 100 1500 | go run 3-encoder2/encoder1.5.go -M <(echo "$MSG" | gzip -c) 2>/dev/null | go run 0-decoder/decoder.go 2>/dev/null | gzip -dc 2>/dev/null)
run_test

TESTNAME="encoder2 + decode"
EXPECTED="$MSG"
RESULT=$(seq 100 1500 | go run 3-encoder2/encoder2.go -M <(echo "$MSG" | gzip -c) 2>/dev/null | go run 0-decoder/decoder.go 2>/dev/null | gzip -dc 2>/dev/null)
run_test

TESTNAME="encoder2 + decode (real-life list)"
EXPECTED="$MSG"
RESULT=$(cat lists-from-internet/Wage.csv | go run 3-encoder2/encoder2.go -M <(echo "$MSG" | gzip -c) 2>/dev/null | go run 0-decoder/decoder.go 2>/dev/null | gzip -dc 2>/dev/null)
run_test

TESTNAME="encoder2 binary + decode"
EXPECTED=$(cat stuff-for-tests/256chars.txt | shasum -a 256)
RESULT=$(seq 100 2148 | go run 3-encoder2/encoder2.go -M stuff-for-tests/256chars.txt 2>/dev/null | go run 0-decoder/decoder.go 2>/dev/null | shasum -a 256)
run_test

TESTNAME="encoder2 + analyze"
EXPECTED="VERDICT: NO"
RESULT=$(seq 100 1500 | go run 3-encoder2/encoder2.go -M <(echo "$MSG" | gzip -c) 2>/dev/null | go run 2-analyze/analyze.go 2>/dev/null)
run_test

TESTNAME="superencoder + decode (no 2nd covert message embedded)"
EXPECTED="$MSG"
RESULT=$(seq 100 1500 | go run 4-superencoder/superencoder.go -M <(echo "$MSG" | gzip -c) 2>/dev/null | go run 0-decoder/decoder.go 2>/dev/null | gzip -dc 2>/dev/null)
run_test

# This will be the 2nd level hidden message, hidden inside the algorithm
SECOND_MSG="2 JAN 8PM BLD 5" # Needs to be about half the size of the original message, plus gzip header length

# encode message into file, so the same file will be read for the 1st and the 2nd message both
TMPFILE_PLAINTEXT=$(mktemp)
TMPFILE_CRYPTTEXT=$(mktemp)
echo "$MSG" | gzip -c >$TMPFILE_PLAINTEXT
seq 100 1500 | go run 4-superencoder/superencoder.go -M $TMPFILE_PLAINTEXT -M2 <(echo "$SECOND_MSG" | gzip -c) 2>/dev/null >$TMPFILE_CRYPTTEXT

# Decode 1st message to file, so we can demonstrate using that in extracting the 2nd text
TMPFILE_DECODEDMSG=$(mktemp)
TESTNAME="superencoder + decode (extracting the 1st hidden message)"
EXPECTED="$MSG"
RESULT=$(cat $TMPFILE_CRYPTTEXT | go run 0-decoder/decoder.go 2>/dev/null | tee $TMPFILE_DECODEDMSG | gzip -dc 2>/dev/null)
run_test

TESTNAME="superencoder + superdecode (extracting the 2nd, meta-hidden message from the previous test's list)"
EXPECTED="$SECOND_MSG"
RESULT=$(cat $TMPFILE_CRYPTTEXT | go run 4-superencoder/superdecoder.go -M $TMPFILE_DECODEDMSG | gzip -dc 2>/dev/null)
run_test

# Clean up
rm "$TMPFILE_DECODEDMSG"
rm "$TMPFILE_CRYPTTEXT"
rm "$TMPFILE_PLAINTEXT"
