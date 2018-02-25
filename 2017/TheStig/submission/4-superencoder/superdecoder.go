package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"sort"
	"strings"
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

	// 2ND: Make a copy of the original encoder's result.
	his_lines := make([]string, len(lines))
	copy(his_lines, lines)

	// ENCODING
	// Sort lines
	sort.Strings(lines)
	// Encode message into lines
	lines, secondary_msg := Decode(lines, his_lines, *message)
	/*
		// Print output to stdout
		for _, l := range lines {
			fmt.Println(l)
		}
	*/
	// Print recovered secondary message to stdout
	fmt.Println(string(secondary_msg))

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
func Decode(in, his_lines []string, message string) (out []string, msg []byte) {

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
	// IN ENCODER: cursor := zeros + rand.Intn(len(in)-ones-zeros)
	// But instead we do: find his original random starting place
	cursor := sort.SearchStrings(in, his_lines[0])

	// DECODER: Secondary cursor to keep track of HIS (the original encoder's) progress
	cursor2 := 1

	// Reset pointer to the beginning of samebits
	ptr = 0

	// The first line is only there to have something to compare the second line to
	in, out = Takeline(in, out, cursor)

	// Variables for decoding:
	// How many bytes are needed? (-1 for the starter, first line)
	var bufsize = (len(in) - 1) / 8
	// If there are remaining lines, add one more byte
	if (len(in)-1)%8 > 0 {
		bufsize++
	}
	msg = make([]byte, bufsize)
	var bit2 uint
	var ch2 uint

	// PASS 2: Encode message and thus decode secondary bits
	for ch := 0; ch < len(message_bytes); ch++ {
		for bit := 0; bit < 8; bit++ {
			// Decrease current count-of-bits, since we're using up one right now
			samebits[ptr]--
			// Zero or one?
			switch message_bytes[ch] & (1 << uint(bit)) {
			case 0:
				// 0 bit - take a line that is "less" than the last one
				if samebits[ptr] > 0 {
					allowed := (cursor - samebits[ptr]) / (samebits[ptr] + 1)
					allowed = allowed >> 1 // half

					// In the ENCODER, it goes like this:
					//   if (allowed > 0 && <-m2) {
					//   	cursor = cursor - allowed
					//   }
					//   rnd := rand.Intn(allowed + 1)
					//   cursor = cursor - 1 - rnd

					// But instead, we find his end result "cursor":
					his_cursor := sort.SearchStrings(in, his_lines[cursor2])
					if allowed > 0 {
						his_rnd := cursor - 1 - his_cursor
						// On which side of "allowed" did he decide?
						if his_rnd >= allowed {
							// Set bit in byte
							msg[ch2] = msg[ch2] | (1 << bit2)
						}
						// Step one bit
						bit2++
						// Step one byte when needed
						if bit2 == 8 {
							bit2 = 0
							ch2++
						}
					}
					cursor = his_cursor
				} else {
					// No bit encoded here, just following his randoms
					cursor = sort.SearchStrings(in, his_lines[cursor2])
					ptr++
				}
			default:
				// 1 bit - take a line that is "more" than the last one
				if samebits[ptr] > 0 {
					allowed := (len(in) - cursor - samebits[ptr]) / (samebits[ptr] + 1)
					allowed = allowed >> 1 // half

					// In the ENCODER, it goes like this:
					//   if (allowed > 0 && <-m2) {
					//   	cursor = cursor + allowed
					//   }
					//   rnd := rand.Intn(allowed + 1)
					//   cursor = cursor + rnd

					// But instead, we find his end result "cursor":
					his_cursor := sort.SearchStrings(in, his_lines[cursor2])
					if allowed > 0 {
						his_rnd := his_cursor - cursor
						// On which side of "allowed" did he decide?
						if his_rnd >= allowed {
							// Set bit in byte
							msg[ch2] = msg[ch2] | (1 << bit2)
						}
						// Step one bit
						bit2++
						// Step one byte when needed
						if bit2 == 8 {
							bit2 = 0
							ch2++
						}
					}
					cursor = his_cursor
				} else {
					// No bit encoded here, just following his randoms
					cursor = sort.SearchStrings(in, his_lines[cursor2])
					ptr++
				}
			}
			in, out = Takeline(in, out, cursor)
			// Sanity check: do we have the same output as the encoder?
			if his_lines[cursor2] != out[len(out)-1] {
				log.Fatal("encoder-decoder inconsistency!")
			}
			// 2ND: he progresses, we progress
			cursor2++
		}
	}

	return
}
