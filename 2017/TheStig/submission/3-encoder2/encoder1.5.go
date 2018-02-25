package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"math/rand"
	"os"
	"sort"
	"strings"
	"time"
)

func main() {
	// Parse command line arguments
	message := flag.String("m", "", "message as a string")
	messagefile := flag.String("M", "", "read message from a file")
	flag.Parse()

	// Read all from stdin
	input, err := ioutil.ReadAll(os.Stdin)
	if err != nil {
		log.Fatal(err)
	}

	// Read the hidden message from a file, if that was the user request
	if *messagefile != "" {
		fil, err := os.Open(*messagefile)
		if err != nil {
			log.Fatal(err)
		}
		msgcontent, err := ioutil.ReadAll(fil)
		if err != nil {
			log.Fatal(err)
		}
		*message = string(msgcontent)
	}

	// Split []byte into lines
	lines := strings.Split(string(input), "\n")
	// Throw away ending empty line
	if lines[len(lines)-1] == "" {
		// Throwing away empty last line
		lines = lines[:len(lines)-1]
	}

	// Seed random in case we need it
	rand.Seed(time.Now().UnixNano())

	// ENCODING
	// Sort lines
	sort.Strings(lines)
	// Throw away unique lines for now
	lines = Deduplicate(lines)
	// Encode message into lines
	lines = Encode(lines, *message)
	// Print output to stdout
	for _, l := range lines {
		fmt.Println(l)
	}

}

// Deduplicate throws away duplicates in a sorted []string
func Deduplicate(lines []string) []string {
	var prev string
	newlines := make([]string, 0, len(lines))
	var count uint
	for i, l := range lines {
		// First line is just something to compare to
		if i == 0 {
			newlines = append(newlines, l)
			prev = l
			continue
		}
		// Same?
		if strings.Compare(prev, l) != 0 {
			// If not, keep it
			newlines = append(newlines, l)
		} else {
			count++
		}
		prev = l
	}
	// Message
	if count > 0 {
		log.Printf("Thrown away %d unique lines", count)
	}
	return newlines
}

// Takes the "cursor"-th line from the "in" []string, appends to the "out" []string, and returns both
func Takeline(in, out []string, cursor int) (in2, out2 []string) {
	// Append the line to the output
	out2 = append(out, in[cursor])
	// Throw away the used up line
	in2 = append(in[:cursor], in[cursor+1:]...)
	return
}

// Encode encodes a message into an array of sorted lines and returns a new array of lines.
func Encode(in []string, message string) (out []string) {

	// PASS 1: Bit stats. Parse the message and gives statistics that we need to do the encoding.
	message_bytes := []byte(message)
	var zeros, ones int
	// samebits will contain how many similar bits are there before a switch in parity
	var samebits = make([]int, 1)
	var ptr int
	for ch := 0; ch < len(message_bytes); ch++ {
		for bit := 0; bit < 8; bit++ {
			switch message_bytes[ch] & (1 << uint(bit)) {
			case 0:
				zeros++
				// Set ptr to even
				if ptr%2 == 1 {
					samebits = append(samebits, 0)
					ptr++
				}
				// Increase counter
				samebits[ptr]++
			default:
				ones++
				// Set ptr to odd
				if ptr%2 == 0 {
					samebits = append(samebits, 0)
					ptr++
				}
				// Increase counter
				samebits[ptr]++
			}
		}
	}

	// Cut leading zero from the beginning of samebits, if any.
	if samebits[0] == 0 {
		samebits = samebits[1:]
	}
	// samebits will contain how many similar bits are there before a switch in parity
	samebits = append(samebits, 0)

	// Sanity check -- do we have enough lines?
	if len(in) < 1+ones+zeros {
		log.Fatalf("Not enough lines in source! (has %d, needs %d)", len(in), 1+ones+zeros)
	}

	// Set a logical cursor so we have enough 1 and 0 "bits" on its sides (lower and higher lines).
	// Start at a random place if we have more than enough lines.
	cursor := zeros + rand.Intn(len(in)-ones-zeros)

	// Reset pointer to the beginning of samebits
	ptr = 0

	// The first line is only there to have something to compare the second line to
	in, out = Takeline(in, out, cursor)

	// PASS 2: Encode message
	for ch := 0; ch < len(message_bytes); ch++ {
		for bit := 0; bit < 8; bit++ {
			// Decrease current count-of-bits, since we're using up one right now
			samebits[ptr]--
			// Zero or one?
			switch message_bytes[ch] & (1 << uint(bit)) {
			case 0:
				// 0 bit - take a line that is "less" than the last one
				if samebits[ptr] > 0 {
					// More bits of the same are coming up -- leave space "above"
					//
					// Same bits (samebits[ptr]): 2
					// Cursor: 3
					//    0 NOT OK: not enough space for next bit
					//    1 NOT OK: not enough space for 2 bits
					//    2 OK
					// => 3 NOT OK: would mean a "1"
					//
					// Also, leave room for following bits, or else they squash into the edge of the range, potentially leaking statistics
					rnd := rand.Intn(cursor - samebits[ptr])
					rnd = rnd / (samebits[ptr] + 1)
					cursor = cursor - 1 - rnd
				} else {
					// Next bit will be different -- leave space "below"
					//
					// Next bits (samebits[ptr]): 2
					// Cursor: 1
					//    0 OK
					// => 1 NOT OK: would mean "1"
					//    2 NOT OK: not enough space for 2 bits
					//    3 NOT OK: not enough space for next bit
					ptr++
					var min int
					if cursor > len(in)-samebits[ptr] {
						min = len(in) - samebits[ptr]
					} else {
						min = cursor
					}
					cursor = rand.Intn(min)
				}
				in, out = Takeline(in, out, cursor)
			default:
				// 1 bit - take a line that is "more" than the last one
				if samebits[ptr] > 0 {
					// More bits of the same are coming up -- leave space "below"
					//
					// Same bits (samebits[ptr]): 2
					// Cursor: 1
					//    0 NOT OK: would mean a "0"
					// => 1 OK
					//    2 NOT OK: not enough space for next 2 bits
					//    3 NOT OK: not enough space for next bit
					//
					// Also, leave room for following bits, or else they squash into the edge of the range, potentially leaking statistics
					rnd := rand.Intn(len(in) - cursor - samebits[ptr])
					rnd = rnd / (samebits[ptr] + 1)
					cursor = cursor + rnd
				} else {
					// Next bit will be different -- leave space "above"
					//
					// Next bits (samebits[ptr]): 2
					// Cursor: 3
					//    0 NOT OK: not enough space for next bit
					//    1 NOT OK: not enough space for next 2 bits
					//    2 NOT OK: would mean "0"
					// => 3 OK
					ptr++
					if cursor > samebits[ptr] {
						cursor = cursor + rand.Intn(len(in)-cursor)
					} else {
						cursor = samebits[ptr] + rand.Intn(len(in)-samebits[ptr])
					}
				}
				in, out = Takeline(in, out, cursor)
			}
		}
	}

	// PASS 3: Emit remaining lines
	// XXX This will decode to some gibberish, but that can be ignored
	if len(in) > 0 {
		log.Printf("Remaining %d lines, emitting them as gibberish", len(in))
		for i := len(in); i > 0; i-- {
			in, out = Takeline(in, out, rand.Intn(i))
		}
	}

	return
}
