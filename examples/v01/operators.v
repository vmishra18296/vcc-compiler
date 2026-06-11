// operators.v — All operators supported by V v0.1
// Demonstrates: arithmetic, comparison, logical, bitwise operators

module main

func demo_arithmetic() {
    let a: i64 = 20
    let b: i64 = 6

    let sum:  i64 = a + b    // 26
    let diff: i64 = a - b    // 14
    let prod: i64 = a * b    // 120
    let quot: i64 = a / b    // 3
    let rem:  i64 = a % b    // 2

    print(sum)
    print(diff)
    print(prod)
    print(quot)
    print(rem)
}

func demo_comparison() {
    let x: i64 = 10
    let y: i64 = 20

    let eq:  bool = x == y   // false
    let ne:  bool = x != y   // true
    let lt:  bool = x < y    // true
    let gt:  bool = x > y    // false
    let lte: bool = x <= y   // true
    let gte: bool = x >= y   // false

    print(eq)
    print(ne)
    print(lt)
}

func demo_logical() {
    let a: bool = true
    let b: bool = false

    let land: bool = a && b    // false
    let lor:  bool = a || b    // true
    let lnot: bool = !a        // false

    print(land)
    print(lor)
    print(lnot)

    // Short-circuit chaining
    let complex: bool = (a || b) && !b
    print(complex)
}

func demo_bitwise() {
    let a: i64 = 0b1010    // 10
    let b: i64 = 0b1100    // 12

    let band: i64  = a & b     // 0b1000 = 8
    let bor:  i64  = a | b     // 0b1110 = 14
    let bxor: i64  = a ^ b     // 0b0110 = 6
    let bnot: i64  = ~a        // bitwise NOT of 10
    let lsh:  i64  = a << 2    // 40
    let rsh:  i64  = a >> 1    // 5

    print(band)
    print(bor)
    print(bxor)
    print(lsh)
    print(rsh)
}

func demo_operator_precedence() {
    // Precedence: * before +
    let r1: i64 = 2 + 3 * 4        // 14 (not 20)

    // Explicit grouping
    let r2: i64 = (2 + 3) * 4      // 20

    // Bitwise before logical
    let r3: i64 = 5 & 3 | 2        // (5&3)|2 = 3
    let r4: i64 = 4 << 1 + 1       // 4 << 2 = 16

    print(r1)
    print(r2)
    print(r3)
    print(r4)
}

func main() {
    demo_arithmetic()
    demo_comparison()
    demo_logical()
    demo_bitwise()
    demo_operator_precedence()
}
