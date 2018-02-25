package main

import(
    "fmt"
    "crypto/rand"
    "crypto/sha256"
)

func xor_byte_arrays(first, second []byte) []byte {
    if len(first) != len(second) {
        panic("Cannot join byte arrays of two differentl lengths.")
    }

    if len(first) != 32 {
        panic("Byte arrays must be 32 bytes long.")
    }

    result := make([]byte, 32)

    for i, _ := range first {
        result[i] = first[i] ^ second[i]
    }

    return result
}


func main() {
    key := make([]byte, 32)
    key2 := make([]byte, 32)
    next := make([]byte, 32)
    start := make([]byte, 1)

    /*
    Get two random keys.
    */
    _, err := rand.Read(key)
    if err != nil {
        panic(err.Error())
    }

    _, err = rand.Read(key2)
    if err != nil {
        panic(err.Error())
    }

    /*
    Get a random starting point.
    */
    _, err = rand.Read(start)
    if err != nil {
        panic(err.Error())
    }


    /*
    Shuffle the keys with XOR
    */
    final := xor_byte_arrays(key, key2)
    key2 = xor_byte_arrays(key2, final)
    final = xor_byte_arrays(key, key2)

    /*
    Generate more bits using our shuffled random key and the SHA256 hash
    function.
    */
    next = final
    count := 0
    for {
        sum := sha256.Sum256(next)
        next = sum[:]

        // Do not print bytes until after start number of iterations.
        if count > int(start[0]) {
            for i, _ := range next {
                fmt.Printf("%d\n", int(next[i]))
            }
        }

        count = count + 1
    }
}
