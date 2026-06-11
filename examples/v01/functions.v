// functions.v — Function definitions and calls using fun keyword
// Demonstrates: func, typed params, return types, forward calls, recursion

module main

// ── Forward-declared functions are called before their definition ─────────────

fun main() {
    // Basic arithmetic functions
    let sum: i64 = add(10, 20)
    print(sum)

    let diff: i64 = sub(50, 15)
    print(diff)

    let prod: i64 = mul(6, 7)
    print(prod)

    // Recursive functions
    let f10: i64 = fib(10)
    print(f10)

    let f5: i64 = fact(5)
    print(f5)

    // String functions
    greet("vikas")
    greet_formal("Dr", "Mishra")

    // Bool return
    let even: bool = is_even(42)
    print(even)

    let prime: bool = is_prime(17)
    print(prime)

    // Chained calls
    let result: i64 = add(mul(3, 4), sub(10, 2))
    print(result)
}

// ── Arithmetic ────────────────────────────────────────────────────────────────

fun add(a: i64, b: i64) -> i64 {
    return a + b
}

fun sub(a: i64, b: i64) -> i64 {
    return a - b
}

fun mul(a: i64, b: i64) -> i64 {
    return a * b
}

fun div(a: i64, b: i64) -> i64 {
    if b == 0 {
        return 0
    }
    return a / b
}

fun mod(a: i64, b: i64) -> i64 {
    return a % b
}

// ── Recursive functions ───────────────────────────────────────────────────────

// Fibonacci — 0 1 1 2 3 5 8 13 21 34 ...
fun fib(n: i64) -> i64 {
    if n <= 1 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

// Factorial
fun fact(n: i64) -> i64 {
    if n <= 1 {
        return 1
    }
    return n * fact(n - 1)
}

// Power: base^exp
fun power(base: i64, exp: i64) -> i64 {
    if exp == 0 {
        return 1
    }
    return base * power(base, exp - 1)
}

// ── String functions ──────────────────────────────────────────────────────────

fun greet(name: string) {
    print("Hello, ")
    print(name)
}

fun greet_formal(title: string, name: string) {
    print("Good day, ")
    print(title)
    print(" ")
    print(name)
}

// ── Boolean functions ─────────────────────────────────────────────────────────

fun is_even(n: i64) -> bool {
    return n % 2 == 0
}

fun is_odd(n: i64) -> bool {
    return n % 2 != 0
}

fun is_positive(n: i64) -> bool {
    return n > 0
}

// Primality test
fun is_prime(n: i64) -> bool {
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

// ── No-return (void) functions ────────────────────────────────────────────────

fun print_separator() {
    print("────────────────────────────────")
}

fun print_n_times(msg: string, n: i64) {
    var i: i64 = 0
    vloop(i < n) {
        print(msg)
        i = i + 1
    }
}
