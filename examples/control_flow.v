module main

import std.io

// ── if / else ────────────────────────────────────────────────────────────────

fn classify_number(n: int) -> str {
    if n < 0 {
        return "negative"
    } else if n == 0 {
        return "zero"
    } else {
        return "positive"
    }
}

fn absolute_value(n: int) -> int {
    if n < 0 {
        return -n
    }
    return n
}

fn max(a: int, b: int) -> int {
    if a >= b {
        return a
    }
    return b
}

// ── while ─────────────────────────────────────────────────────────────────────

fn sum_to(n: int) -> int {
    var total = 0
    var i = 1
    while i <= n {
        total += i
        i += 1
    }
    return total
}

fn count_digits(n: int) -> int {
    if n == 0 {
        return 1
    }
    var count = 0
    var x = absolute_value(n)
    while x > 0 {
        count += 1
        x = x / 10
    }
    return count
}

// ── break / continue ─────────────────────────────────────────────────────────

fn first_multiple_of_7(limit: int) -> int {
    var n = 1
    while n <= limit {
        if n % 7 == 0 {
            return n
        }
        n += 1
    }
    return -1     // not found
}

fn sum_odd_up_to(n: int) -> int {
    var total = 0
    var i = 1
    while i <= n {
        if i % 2 == 0 {
            i += 1
            continue        // skip even numbers
        }
        total += i
        i += 1
    }
    return total
}

// ── for … in ──────────────────────────────────────────────────────────────────

fn sum_range(from: int, to: int) -> int {
    var total = 0
    for i in range(from, to) {
        total += i
    }
    return total
}

fn main() {
    // if / else
    io.println(classify_number(-5))   // negative
    io.println(classify_number(0))    // zero
    io.println(classify_number(42))   // positive

    // while
    io.println("sum 1..10 = " + sum_to(10).to_string())         // 55
    io.println("digits(12345) = " + count_digits(12345).to_string())  // 5

    // break / continue
    io.println("first multiple of 7 <= 50: " + first_multiple_of_7(50).to_string())  // 7
    io.println("sum of odds up to 10: " + sum_odd_up_to(10).to_string())              // 25

    // for … in
    io.println("sum [1,5) = " + sum_range(1, 5).to_string())    // 10

    // max
    io.println("max(8, 3) = " + max(8, 3).to_string())          // 8
}
