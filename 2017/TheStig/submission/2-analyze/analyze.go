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
	is_verbose := flag.Bool("v", false, "verbose")
	flag.Parse()

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

	// ANALYZING
	Analyze(lines, *is_verbose)

}

// Analyze a list to determine if it's an output of Comrade #23's Encode, even if the encoding contains random-looking (or encrypted) data
func Analyze(in []string, is_verbose bool) {

	// Restore a sorted copy of the lines
	sorted := make([]string, len(in))
	copy(sorted, in)
	sort.Strings(sorted)

	// Turn it into a hash table, for quick searching
	hash := make(map[string]int)
	for i, s := range sorted {
		hash[s] = i
	}

	// Analyze
	var prev, consecutive_match, same, different int
	for i, line := range in {
		// First line is just something to compare to
		if i > 0 {
			diff := prev - hash[line]
			if is_verbose {
				fmt.Printf("%#v ", diff)
			}
			// Strangely, no Abs() in go, just for float64
			if diff < 0 {
				diff = -diff
			}
			if diff == 1 {
				consecutive_match++
				same++
			} else if diff == i {
				consecutive_match++
				different++
			} else {
				//break
			}
		}
		prev = hash[line]
	}

	// Print evaluation
	if is_verbose {
		fmt.Println()
	}
	log.Printf("The first %d lines might be cover, with %d consecutive bits being the same and %d being different.", consecutive_match, same, different)
	if consecutive_match < 8 {
		fmt.Println("VERDICT: NO")
		log.Println("NO - Not enough cover-like lines even for 1 byte of information.")
	} else if consecutive_match < 24 {
		fmt.Println("VERDICT: NO")
		log.Printf("NO - Not enough cover-like lines even for %d bytes of information.\n", (consecutive_match) >> 3 + 1)
	} else if different == 0 {
		fmt.Println("VERDICT: NO")
		log.Println("NO - Hidden text is all 1s or 0s (there are no different bits), this is just a basic ordered file.")
	} else {
		fmt.Println("VERDICT: YES")
		log.Println("YES - This is probably a cover, try to decode with 'decoder'!")
	}
}
