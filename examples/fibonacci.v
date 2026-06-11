module main

import std.io

/// Compute the nth Fibonacci number recursively.
fn fibonacci(n: i64) -> i64 {
    if n <= 1 {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

/// Compute Fibonacci iteratively (O(n) time, O(1) space).
fn fibonacci_iter(n: i64) -> i64 {
    if n <= 1 {
        return n
    }
    var a: i64 = 0
    var b: i64 = 1
    var i: i64 = 2
    while i <= n {
        let tmp = a + b
        a = b
        b = tmp
        i = i + 1
    }
    return b
}

fn main() {
    let n: i64 = 10
    let result_rec  = fibonacci(n)
    let result_iter = fibonacci_iter(n)
    io.println("fib_recursive(10) = " + result_rec.to_string())
    io.println("fib_iterative(10) = " + result_iter.to_string())
}
