module main

import std.io

// ── Basic function forms ──────────────────────────────────────────────────────

// No parameters, no return value
fn print_separator() {
    io.println("──────────────────────────────")
}

// Single parameter
fn double(n: int) -> int {
    return n * 2
}

// Multiple parameters
fn add(a: int, b: int) -> int {
    return a + b
}

// Multiple parameters with different types
fn repeat(s: str, times: int) -> str {
    var result = ""
    var i = 0
    while i < times {
        result = result + s
        i += 1
    }
    return result
}

// ── Recursion ────────────────────────────────────────────────────────────────

fn factorial(n: int) -> int {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

fn fibonacci(n: int) -> int {
    if n <= 1 {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

// Recursive power: base ^ exp
fn power(base: int, exp: int) -> int {
    if exp == 0 {
        return 1
    }
    if exp % 2 == 0 {
        let half = power(base, exp / 2)
        return half * half
    }
    return base * power(base, exp - 1)
}

// ── Mutual recursion (forward references) ────────────────────────────────────
// is_even calls is_odd, which is defined below it — this is legal in V because
// all module-level functions are pre-declared before their bodies are checked.

fn is_even(n: int) -> bool {
    if n == 0 {
        return true
    }
    return is_odd(n - 1)
}

fn is_odd(n: int) -> bool {
    if n == 0 {
        return false
    }
    return is_even(n - 1)
}

// ── Higher-order concepts (function as value) ─────────────────────────────────

// Functions can be passed as arguments (first-class values in V 0.2+).
// For now, demonstrate strategy via branching.

fn apply_twice(x: int, fn_selector: int) -> int {
    if fn_selector == 0 {
        return double(double(x))
    }
    return add(add(x, 1), 1)
}

// ── GCD and LCM ──────────────────────────────────────────────────────────────

fn gcd(a: int, b: int) -> int {
    if b == 0 {
        return a
    }
    return gcd(b, a % b)
}

fn lcm(a: int, b: int) -> int {
    return (a / gcd(a, b)) * b
}

// ── String utilities ─────────────────────────────────────────────────────────

fn greet(name: str) -> str {
    return "Hello, " + name + "!"
}

fn is_palindrome_str(s: str) -> bool {
    // Simplified: checks first == last character (real impl needs indexing)
    // This demonstrates the pattern; full string indexing is a v0.2 feature.
    return true  // placeholder
}

// ── main ─────────────────────────────────────────────────────────────────────

fn main() {
    print_separator()
    io.println("Basic functions")
    print_separator()
    io.println("double(7)    = " + double(7).to_string())          // 14
    io.println("add(3,4)     = " + add(3, 4).to_string())          // 7
    io.println("repeat(*,3)  = " + repeat("*", 3))                 // ***

    print_separator()
    io.println("Recursion")
    print_separator()
    io.println("5!           = " + factorial(5).to_string())       // 120
    io.println("fib(8)       = " + fibonacci(8).to_string())       // 21
    io.println("2^10         = " + power(2, 10).to_string())       // 1024

    print_separator()
    io.println("Mutual recursion")
    print_separator()
    io.println("is_even(4)   = " + is_even(4).to_string())         // true
    io.println("is_odd(7)    = " + is_odd(7).to_string())          // true

    print_separator()
    io.println("GCD / LCM")
    print_separator()
    io.println("gcd(48,18)   = " + gcd(48, 18).to_string())        // 6
    io.println("lcm(4,6)     = " + lcm(4, 6).to_string())          // 12

    print_separator()
    io.println(greet("V programmer"))
}
