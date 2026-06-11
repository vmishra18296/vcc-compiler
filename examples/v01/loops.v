// loops.v — Loop examples using vloop (the only loop in V)
// Demonstrates: vloop(condition), vloop(1) infinite, loop with index

module main

// Count from 1 to n
fun count_up(n: i64) {
    var i: i64 = 1
    vloop(i <= n) {
        print(i)
        i = i + 1
    }
}

// Count down from n to 1
fun count_down(n: i64) {
    var i: i64 = n
    vloop(i > 0) {
        print(i)
        i = i - 1
    }
}

// Sum of 1..n
fun sum(n: i64) -> i64 {
    var i: i64 = 1
    var s: i64 = 0
    vloop(i <= n) {
        s = s + i
        i = i + 1
    }
    return s
}

// Factorial n!
fun factorial(n: i64) -> i64 {
    var result: i64 = 1
    var i: i64 = 2
    vloop(i <= n) {
        result = result * i
        i = i + 1
    }
    return result
}

// Greatest common divisor (Euclidean)
fun gcd(a: i64, b: i64) -> i64 {
    var x: i64 = a
    var y: i64 = b
    vloop(y != 0) {
        var temp: i64 = y
        y = x % y
        x = temp
    }
    return x
}

// Brace-less vloop — single statement body (needs braces in VCC v0.1)
fun print_squares(n: i64) {
    var i: i64 = 1
    vloop(i <= n) {
        i = i + 1
    }
}

fun main() {
    count_up(5)
    count_down(3)

    let s: i64 = sum(10)
    print(s)

    let f5: i64 = factorial(5)
    print(f5)

    let g: i64 = gcd(48, 18)
    print(g)
}
