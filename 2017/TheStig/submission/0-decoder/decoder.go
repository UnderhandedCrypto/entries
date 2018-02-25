package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"strings"
)

func main() {
	// Read all from stdin
	input, err := ioutil.ReadAll(os.Stdin)
	if err != nil {
		log.Fatal(err)
	}

	// Split []byte into lines
	lines := strings.Split(string(input), "\n")
	// Throw away ending empty line
	if lines[len(lines)-1] == "" {
		// Throwing away empty last line
		lines = lines[:len(lines)-1]
	}

	// DECODING
	fmt.Print(string(Decode(lines)))
}

// Decode takes an array of lines, and extracts the hidden message.
func Decode(in []string) []byte {
	// How many bytes are needed? (-1 for the starter, first line)
	var bufsize = (len(in) - 1) / 8
	// If there are remaining lines, add one more byte
	if (len(in)-1)%8 > 0 {
		bufsize++
	}
	var msg = make([]byte, bufsize)
	var bit uint
	var ch uint
	var prev string
	// Take each line
	for i, l := range in {
		// First line is just something to compare to
		if i == 0 {
			prev = l
			continue
		}
		// Recover next bit by comparing the previous and the current line
		if strings.Compare(prev, l) < 0 {
			// Set bit in byte
			msg[ch] = msg[ch] | (1 << bit)
		}
		// Step one bit
		prev = l
		bit++
		// Step one byte when needed
		if bit == 8 {
			bit = 0
			ch++
		}
	}
	return msg
}
