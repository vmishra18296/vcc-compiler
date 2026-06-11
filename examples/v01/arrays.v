// arrays.v — Array literals and operations
// Demonstrates: array[...], push, pop, insert, remove, size, clear

module main

fun main() {
    // Integer array — type inferred as array[i64]
    nums = array[1, 2, 3, 4, 5]

    // String array — type inferred as array[string]
    names = array["vikas", "mishra", "john", "doe"]

    // Character array — type inferred as array[char]
    vowels = array['a', 'e', 'i', 'o', 'u']

    // Float array
    temps = array[36.5, 37.0, 38.2, 39.1]

    // Bool array
    flags = array[true, false, true, true, false]

    // Single element arrays
    one = array[42]
    greeting = array["hello"]

    // Array operations
    nums.push(6)           // append 6  → [1,2,3,4,5,6]
    nums.insert(0, 0)      // insert 0 at index 0 → [0,1,2,3,4,5,6]
    nums.remove(3)         // remove element at index 3 → [0,1,2,4,5,6]
    nums.pop()             // remove last → [0,1,2,4,5]

    let sz: i64 = nums.size()
    print(sz)

    names.push("smith")
    let nsz: i64 = names.size()
    print(nsz)

    // Clear all elements
    flags.clear()
    let fsz: i64 = flags.size()
    print(fsz)              // 0
}
