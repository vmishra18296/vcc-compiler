module main

import std.io

// ── Built-in types ───────────────────────────────────────────────────────────

fn show_types() {
    // int — 64-bit signed integer
    let age: int = 42
    let negative: int = -17
    let hex_color: int = 0xFF5733
    let binary_flags: int = 0b1010_1100

    // float — 64-bit floating-point
    let pi: float = 3.14159265358979
    let avogadro: float = 6.022e23
    let electron_mass: float = 9.109e-31

    // bool
    let is_valid: bool = true
    let has_error: bool = false

    // string — UTF-8
    let greeting: str = "Hello, World!"
    let escaped: str = "line1\nline2\ttabbed"
    let quoted: str = "she said \"hi\""

    io.println("age       = " + age.to_string())
    io.println("pi        = " + pi.to_string())
    io.println("is_valid  = " + is_valid.to_string())
    io.println("greeting  = " + greeting)
}

// ── Operators ────────────────────────────────────────────────────────────────

fn show_operators() {
    let a: int = 10
    let b: int = 3

    // Arithmetic
    io.println("+  : " + (a + b).to_string())    // 13
    io.println("-  : " + (a - b).to_string())    // 7
    io.println("*  : " + (a * b).to_string())    // 30
    io.println("/  : " + (a / b).to_string())    // 3  (integer division)
    io.println("%  : " + (a % b).to_string())    // 1

    // Comparison (all return bool)
    io.println("== : " + (a == b).to_string())   // false
    io.println("!= : " + (a != b).to_string())   // true
    io.println("<  : " + (a < b).to_string())    // false
    io.println(">  : " + (a > b).to_string())    // true
    io.println("<= : " + (a <= b).to_string())   // false
    io.println(">= : " + (a >= b).to_string())   // true

    // Logical
    let x = a > 0 && b > 0     // true
    let y = a < 0 || b > 0     // true
    let z = !x                  // false
    io.println("&& : " + x.to_string())
    io.println("|| : " + y.to_string())
    io.println("!  : " + z.to_string())

    // Precedence
    let expr1 = 1 + 2 * 3         // 7, not 9
    let expr2 = (1 + 2) * 3       // 9
    let expr3 = a > 0 && b < 10   // true
    io.println("1 + 2 * 3   = " + expr1.to_string())
    io.println("(1 + 2) * 3 = " + expr2.to_string())
}

// ── Variables ────────────────────────────────────────────────────────────────

fn show_variables() {
    // let — immutable
    let immutable = 100
    // immutable = 200  // compile error: cannot assign to immutable binding

    // var — mutable
    var counter = 0
    counter = counter + 1
    counter += 5
    counter -= 2
    io.println("counter = " + counter.to_string())   // 4

    // const — compile-time constant
    const MAX: int = 1024
    const GREETING: str = "Hello"
    io.println("MAX = " + MAX.to_string())
}

fn main() {
    show_types()
    show_operators()
    show_variables()
}
