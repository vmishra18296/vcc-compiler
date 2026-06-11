// variables.v — Variable declarations and type inference
// Demonstrates: let, const, type inference, all primitive types

module main

// Constants — cannot be modified
const NAME   = "vikas"
const PI     = 3.1415926
const MAX    = 100
const FLAG   = true

func main() {
    // Inferred types from literals
    name   = "John"       // string
    age    = 25           // int
    salary = 2500.50      // float
    active = true         // bool

    // Explicit type annotations
    let a: i32    = 10
    let b: f64    = 3.14
    let c: bool   = true
    let d: char   = 'A'
    let e: string = "Hello"

    // All integer widths
    let x8:  i8  = 127
    let x16: i16 = 32767
    let x32: i32 = 2147483647
    let x64: i64 = 9223372036854775807

    // Unsigned integers
    let u8v:  u8  = 255
    let u16v: u16 = 65535
    let u32v: u32 = 4294967295
    let u64v: u64 = 9223372036854775807  // max representable in i64 literal

    // Floats
    let f32v: f32 = 3.14
    let f64v: f64 = 2.718281828

    print(name)
    print(age)
    print(PI)
}
