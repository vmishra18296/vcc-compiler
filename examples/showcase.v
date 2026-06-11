module main

import std.io

// ────────────────────────────────────────────────────────────────────────────
// Comprehensive V language showcase — one file that exercises every feature
// covered by the Phase 1 grammar.
// ────────────────────────────────────────────────────────────────────────────

// ── Constants ────────────────────────────────────────────────────────────────
const MAX_ITERATIONS: int = 1000
const EPSILON: float = 1.0e-9
const APP_NAME: str = "V Showcase"

// ── Simple arithmetic ────────────────────────────────────────────────────────

fn add(a: int, b: int) -> int { return a + b }
fn sub(a: int, b: int) -> int { return a - b }
fn mul(a: int, b: int) -> int { return a * b }
fn div_int(a: int, b: int) -> int { return a / b }
fn mod_int(a: int, b: int) -> int { return a % b }

// ── Boolean logic ────────────────────────────────────────────────────────────

fn both_positive(a: int, b: int) -> bool {
    return a > 0 && b > 0
}

fn either_zero(a: int, b: int) -> bool {
    return a == 0 || b == 0
}

fn in_range(x: int, lo: int, hi: int) -> bool {
    return x >= lo && x <= hi
}

// ── String construction ───────────────────────────────────────────────────────

fn make_header(title: str) -> str {
    return "=== " + title + " ==="
}

fn number_to_label(n: int) -> str {
    if n < 0      { return "negative" }
    if n == 0     { return "zero" }
    if n < 10     { return "small" }
    if n < 100    { return "medium" }
    return "large"
}

// ── Iterative algorithms ──────────────────────────────────────────────────────

fn sum(n: int) -> int {
    var total = 0
    var i = 1
    while i <= n {
        total += i
        i += 1
    }
    return total
}

fn product(n: int) -> int {
    var result = 1
    var i = 1
    while i <= n {
        result *= i
        i += 1
    }
    return result
}

fn count_primes_up_to(limit: int) -> int {
    var count = 0
    var n = 2
    while n <= limit {
        if is_prime(n) {
            count += 1
        }
        n += 1
    }
    return count
}

// ── Recursive algorithms ──────────────────────────────────────────────────────

fn is_prime(n: int) -> bool {
    if n < 2  { return false }
    if n == 2 { return true  }
    if n % 2 == 0 { return false }
    var d = 3
    while d * d <= n {
        if n % d == 0 { return false }
        d += 2
    }
    return true
}

fn gcd(a: int, b: int) -> int {
    if b == 0 { return a }
    return gcd(b, a % b)
}

fn fibonacci(n: int) -> int {
    if n <= 1 { return n }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

fn power(base: int, exp: int) -> int {
    if exp == 0 { return 1 }
    if exp % 2 == 0 {
        let half = power(base, exp / 2)
        return half * half
    }
    return base * power(base, exp - 1)
}

// ── Float arithmetic ──────────────────────────────────────────────────────────

fn circle_area(radius: float) -> float {
    const PI: float = 3.14159265358979
    return PI * radius * radius
}

fn celsius_to_fahrenheit(c: float) -> float {
    return c * 9.0 / 5.0 + 32.0
}

// ── Nested control flow ───────────────────────────────────────────────────────

fn fizzbuzz(n: int) -> str {
    if n % 15 == 0 { return "FizzBuzz" }
    if n % 3 == 0  { return "Fizz"     }
    if n % 5 == 0  { return "Buzz"     }
    return n.to_string()
}

fn collatz_steps(n: int) -> int {
    var steps = 0
    var x = n
    while x != 1 {
        if x % 2 == 0 {
            x = x / 2
        } else {
            x = x * 3 + 1
        }
        steps += 1
    }
    return steps
}

// ── Mutual recursion ─────────────────────────────────────────────────────────

fn is_even(n: int) -> bool {
    if n == 0 { return true  }
    return is_odd(n - 1)
}

fn is_odd(n: int) -> bool {
    if n == 0 { return false }
    return is_even(n - 1)
}

// ── Main ─────────────────────────────────────────────────────────────────────

fn main() {
    io.println(make_header(APP_NAME))

    // Types
    io.println("")
    io.println("── Types ──")
    let i: int    = 42
    let f: float  = 2.718
    let b: bool   = true
    let s: str    = "hello"
    io.println("int   = " + i.to_string())
    io.println("float = " + f.to_string())
    io.println("bool  = " + b.to_string())
    io.println("str   = " + s)

    // Operators
    io.println("")
    io.println("── Operators ──")
    io.println("7 + 3  = " + add(7, 3).to_string())
    io.println("7 - 3  = " + sub(7, 3).to_string())
    io.println("7 * 3  = " + mul(7, 3).to_string())
    io.println("7 / 3  = " + div_int(7, 3).to_string())
    io.println("7 % 3  = " + mod_int(7, 3).to_string())

    // Booleans
    io.println("")
    io.println("── Boolean logic ──")
    io.println("both_positive(3,4) = " + both_positive(3, 4).to_string())
    io.println("either_zero(0,5)   = " + either_zero(0, 5).to_string())
    io.println("in_range(5,1,10)   = " + in_range(5, 1, 10).to_string())

    // Strings
    io.println("")
    io.println("── Strings ──")
    io.println(number_to_label(-3))
    io.println(number_to_label(0))
    io.println(number_to_label(7))
    io.println(number_to_label(42))
    io.println(number_to_label(999))

    // Loops
    io.println("")
    io.println("── Loops ──")
    io.println("sum(100)     = " + sum(100).to_string())
    io.println("10!          = " + product(10).to_string())
    io.println("primes ≤ 30  = " + count_primes_up_to(30).to_string())

    // Recursion
    io.println("")
    io.println("── Recursion ──")
    io.println("fib(10)      = " + fibonacci(10).to_string())
    io.println("2^16         = " + power(2, 16).to_string())
    io.println("gcd(48,18)   = " + gcd(48, 18).to_string())
    io.println("is_prime(97) = " + is_prime(97).to_string())

    // Float
    io.println("")
    io.println("── Float ──")
    io.println("area(r=5)       = " + circle_area(5.0).to_string())
    io.println("0°C in °F       = " + celsius_to_fahrenheit(0.0).to_string())
    io.println("100°C in °F     = " + celsius_to_fahrenheit(100.0).to_string())

    // FizzBuzz
    io.println("")
    io.println("── FizzBuzz (1..20) ──")
    var n = 1
    while n <= 20 {
        io.print(fizzbuzz(n) + " ")
        n += 1
    }
    io.println("")

    // Collatz
    io.println("")
    io.println("── Collatz steps ──")
    io.println("collatz(27) = " + collatz_steps(27).to_string() + " steps")

    // Mutual recursion
    io.println("")
    io.println("── Mutual recursion ──")
    io.println("is_even(10) = " + is_even(10).to_string())
    io.println("is_odd(7)   = " + is_odd(7).to_string())

    io.println("")
    io.println(make_header("done"))
}
