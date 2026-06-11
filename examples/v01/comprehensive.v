// comprehensive.v — Full V v0.1 showcase
// Demonstrates all language features in a single file.
// This is the reference program for the VCC v0.1 spec.

module main

import io

// ── Constants ─────────────────────────────────────────────────────────────────
const PI      = 3.1415926
const E       = 2.7182818
const MAX_INT = 9223372036854775807
const APP     = "VCC Demo"

// ── Forward references — main() calls everything defined below ───────────────

func main() {
    print(APP)
    print("────────────────────────────────────")

    // Variables
    name   = "vikas"
    age    = 30
    height = 5.9
    active = true

    print(name)
    print(age)

    // Conditions: if / ef / el
    if age > 60 {
        print("senior")
    }
    ef age > 30 {
        print("adult")
    }
    ef age > 18 {
        print("young adult")
    }
    el {
        print("minor")
    }

    // vloop
    var i: i64 = 0
    vloop(i < 5) {
        print(i)
        i = i + 1
    }

    // match
    let status: i64 = 1
    match status {
        0 => print("OFF")
        1 => print("ON")
        _ => print("UNKNOWN")
    }

    // Arrays
    nums   = array[10, 20, 30, 40, 50]
    strs   = array["alpha", "beta", "gamma"]
    bools  = array[true, false, true]

    nums.push(60)
    let sz: i64 = nums.size()
    print(sz)

    // Function calls
    let s: i64 = sum(100)
    print(s)

    let f7: i64 = fib(7)
    print(f7)

    greet(name)

    let p: bool = is_prime(97)
    print(p)

    // Operators
    let bits: i64 = 0b1111 & 0b1010   // 10
    let shifted: i64 = bits << 2       // 40
    let xored: i64   = bits ^ 0b0101   // 15

    print(bits)
    print(shifted)
    print(xored)
}

// ── Utility functions ─────────────────────────────────────────────────────────

func sum(n: i64) -> i64 {
    var total: i64 = 0
    var i: i64     = 1
    vloop(i <= n) {
        total = total + i
        i = i + 1
    }
    return total
}

func fib(n: i64) -> i64 {
    if n <= 1 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

func greet(name: string) {
    print("Hello, ")
    print(name)
}

func is_prime(n: i64) -> bool {
    if n < 2 {
        return false
    }
    var i: i64 = 2
    vloop(i * i <= n) {
        if n % i == 0 {
            return false
        }
        i = i + 1
    }
    return true
}

func abs(n: i64) -> i64 {
    if n < 0 {
        return 0 - n
    }
    return n
}

func max(a: i64, b: i64) -> i64 {
    if a > b {
        return a
    }
    return b
}

func min(a: i64, b: i64) -> i64 {
    if a < b {
        return a
    }
    return b
}

func clamp(val: i64, lo: i64, hi: i64) -> i64 {
    if val < lo {
        return lo
    }
    if val > hi {
        return hi
    }
    return val
}
