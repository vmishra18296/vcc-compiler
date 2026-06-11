# V Language — Syntax Reference

This document is the **human-readable companion** to `grammar.md`.  It covers
every syntactic construct with annotated examples and covers the decisions that
matter most for a compiler author.

---

## 1. Lexical conventions

### 1.1 Source encoding

V source files are **UTF-8** encoded.  The compiler accepts both `LF` (`\n`)
and `CRLF` (`\r\n`) line endings.

### 1.2 Whitespace and statement termination

V does **not** require semicolons.  Each statement is delimited by the natural
boundaries of its construct (closing `}`, end-of-line for expression
statements, etc.).

```v
let x = 10        // OK — no semicolon needed
let y = 20        // OK
```

### 1.3 Comments

```v
// Single-line comment — extends to end of line

/* Multi-line block comment
   Block comments may be /* nested */ freely */
```

### 1.4 Identifiers

Identifiers begin with a letter (`a–z`, `A–Z`) or underscore (`_`), followed
by zero or more letters, digits (`0–9`), or underscores.

```v
x          _temp       myVariable    count_2
```

Identifiers are **case-sensitive**: `Foo` and `foo` are distinct.

### 1.5 Keywords

The following words are reserved and may not be used as identifiers:

```
fn   let   var   const   return   if   else   while   for   in
break   continue   pub   mut   self   import   module
struct   enum   interface   type   as   match   true   false   nil
```

---

## 2. Types

### 2.1 Primitive types

| Type     | Literals              | Example           |
|----------|-----------------------|-------------------|
| `int`    | decimal, hex, binary  | `42`, `0xFF`, `0b1010` |
| `float`  | decimal with `.` or `e`| `3.14`, `1.0e-9` |
| `bool`   | `true` / `false`      | `true`            |
| `string` | double-quoted UTF-8   | `"hello, world"`  |

### 2.2 Numeric suffixes (future)

No suffixes in V 0.1.  All integer literals default to `int` (64-bit) and all
float literals default to `float` (64-bit).  Explicit casts use `as`:

```v
let small = 255 as i8
let pi    = 3.14159 as f32
```

### 2.3 Integer literals

```v
let decimal  = 1_000_000    // digit separators (future v0.2)
let hex      = 0xFF
let binary   = 0b1101_0010
let octal    = 0o755
```

### 2.4 Float literals

```v
let pi      = 3.14159
let avogadro = 6.022e23
let tiny    = 1.0e-9
```

### 2.5 String literals and escape sequences

```v
let greeting = "Hello, World!"
let newline  = "line1\nline2"
let tab      = "col1\tcol2"
let quote    = "she said \"hi\""
let slash    = "C:\\path"
let hex_char = "\x41"           // 'A'
```

### 2.6 Boolean literals

```v
let yes = true
let no  = false
```

### 2.7 Nil literal

`nil` is the zero value for pointer and optional types.

```v
let ptr: *int = nil
```

---

## 3. Variable declarations

### 3.1 `let` — immutable binding

```v
let name = "Alice"          // type inferred as string
let age: int = 30           // explicit type annotation
let pi: float = 3.14159
```

Attempting to reassign a `let` binding is a compile-time error:

```v
let x = 10
x = 20      // error: cannot assign to immutable binding 'x'
```

### 3.2 `var` — mutable binding

```v
var counter = 0
counter = counter + 1       // OK — var is mutable
```

### 3.3 `const` — compile-time constant

```v
const MAX_SIZE: int = 1024
const PI: float = 3.14159265358979
```

Constants must be initialised with a compile-time-evaluable expression.
They are always immutable and may be used as array sizes.

### 3.4 Type inference

When a type annotation is omitted, the compiler infers the type from the
right-hand side:

```v
let x = 42       // int
let f = 0.5      // float
let s = "hello"  // string
let b = true     // bool
```

---

## 4. Operators

### 4.1 Arithmetic

| Operator | Meaning         | Example          |
|----------|-----------------|------------------|
| `+`      | Addition        | `x + y`          |
| `-`      | Subtraction     | `x - y`          |
| `*`      | Multiplication  | `x * y`          |
| `/`      | Division        | `x / y`          |
| `%`      | Remainder       | `x % y`          |
| `-` (unary) | Negation     | `-x`             |

Integer division truncates toward zero.  Division by zero is a runtime error.

### 4.2 Comparison

All comparison operators return `bool`.

| Operator | Meaning              |
|----------|----------------------|
| `==`     | Equal                |
| `!=`     | Not equal            |
| `<`      | Less than            |
| `>`      | Greater than         |
| `<=`     | Less than or equal   |
| `>=`     | Greater than or equal|

### 4.3 Logical

| Operator | Meaning     | Short-circuits |
|----------|-------------|----------------|
| `&&`     | Logical AND | Yes            |
| `\|\|`   | Logical OR  | Yes            |
| `!`      | Logical NOT | N/A            |

```v
let ok = x > 0 && x < 100
let either = a == 0 || b == 0
let flipped = !ok
```

### 4.4 Assignment operators

```v
x += 5      // x = x + 5
x -= 3      // x = x - 3
x *= 2      // x = x * 2
x /= 4      // x = x / 4
x %= 7      // x = x % 7
```

### 4.5 Operator precedence (highest to lowest)

```
Postfix   ()  []  .
Unary     -   !
Mult      *   /   %
Add       +   -
Compare   <   >   <=  >=
Equality  ==  !=
And       &&
Or        ||
Assign    =   +=  -=  *=  /=  %=
```

```v
let result = 1 + 2 * 3         // 7  (multiplication first)
let cmp    = a > 0 && b < 10   // comparison before &&
let assign = x = y = 5         // right-associative: y=5 then x=5
```

---

## 5. Control flow

### 5.1 `if` / `else`

```v
if condition {
    // executed when condition is true
}

if condition {
    // true branch
} else {
    // false branch
}

if x < 0 {
    // negative
} else if x == 0 {
    // zero
} else {
    // positive
}
```

The condition must have type `bool`.  Parentheses around the condition are
**optional**.

### 5.2 `while` loop

```v
var i = 0
while i < 10 {
    // body
    i += 1
}
```

`break` exits the nearest enclosing loop.  
`continue` skips to the next iteration.

```v
while true {
    if done {
        break
    }
    if skip_this {
        continue
    }
    // process
}
```

### 5.3 `for … in` loop

Iterates over a range or collection.

```v
for item in collection {
    // item is the current element
}
```

Numeric range (using the built-in `range` function):

```v
for i in range(0, 10) {
    // i = 0, 1, …, 9
}
```

---

## 6. Functions

### 6.1 Declaration

```v
fn add(a: int, b: int) -> int {
    return a + b
}
```

### 6.2 No return value (unit)

```v
fn greet(name: string) {
    // implicit unit return
}
```

### 6.3 `return` statement

```v
fn abs(x: int) -> int {
    if x < 0 {
        return -x
    }
    return x
}
```

A bare `return` (no expression) is valid in a unit function.

### 6.4 Forward declarations

Functions are pre-declared at module scope.  A function may call another
function that appears later in the file:

```v
fn is_even(n: int) -> bool {
    if n == 0 { return true }
    return is_odd(n - 1)     // OK — is_odd defined later
}

fn is_odd(n: int) -> bool {
    if n == 0 { return false }
    return is_even(n - 1)
}
```

### 6.5 Mutable parameters

Parameters are immutable by default.  Prefix with `mut` to allow modification:

```v
fn increment(mut n: int) -> int {
    n += 1
    return n
}
```

### 6.6 Public functions

Use `pub` to export a function from a module:

```v
pub fn square(x: int) -> int {
    return x * x
}
```

---

## 7. Modules and imports

Every file belongs to a module declared at the top:

```v
module math

pub fn gcd(a: int, b: int) -> int { … }
```

Importing another module:

```v
import std.io
import math.utils as mu

fn main() {
    io.println("hello")
    let g = mu.gcd(12, 8)
}
```

---

## 8. Types — advanced

### 8.1 Pointer types

```v
let p: *int = nil        // immutable pointer to int
let q: *mut int = nil    // mutable pointer to int
```

### 8.2 Slice types

```v
let bytes: [u8]          // dynamically-sized slice of u8
```

### 8.3 Array types

```v
let buf: [u8; 128]       // fixed-size array: 128 u8 elements
```

### 8.4 Function types

```v
let transform: fn(int) -> int = square
```

### 8.5 Casting

Use `as` for explicit type conversion:

```v
let n: int   = 300
let b: u8    = n as u8      // truncation to 8 bits → 44
let f: float = n as float   // 300.0
```

---

## 9. Structs

```v
struct Point {
    pub x: float,
    pub y: float,
}

fn distance(a: Point, b: Point) -> float {
    let dx = a.x - b.x
    let dy = a.y - b.y
    return (dx * dx + dy * dy) as float   // future: sqrt
}
```

---

## 10. Enums

```v
enum Direction {
    North,
    South,
    East,
    West,
}

enum Result {
    Ok(int),
    Err(string),
}
```

---

## 11. Error handling (future)

In V 0.x, errors are returned as values.  A dedicated `Result<T, E>` type and
`?` propagation operator are planned for v0.3.

---

## 12. Compiler options relevant to syntax

| Flag            | Effect                                     |
|-----------------|--------------------------------------------|
| `--dump-tokens` | Print the token stream and exit            |
| `--dump-ast`    | Print the AST in tree form and exit        |
| `--dump-ir`     | Print VCC IR and exit                      |
| `--no-codegen`  | Run lexer + parser + sema only (lint mode) |

---

## 13. Syntax quick-reference card

```v
// ── Declarations ──────────────────────────────────────────────────────────────
module name
import path.to.module
import path.to.module as alias

let name: Type = expr       // immutable
var name: Type = expr       // mutable
const NAME: Type = expr     // compile-time constant

pub fn name(param: Type, …) -> ReturnType {
    body
}

struct Name { pub field: Type, … }

enum Name { Variant1, Variant2(Type), … }

type Alias = ExistingType

// ── Statements ────────────────────────────────────────────────────────────────
if expr { … } else if expr { … } else { … }
while expr { … }
for var in iterable { … }
return expr
break
continue

// ── Operators (high → low precedence) ────────────────────────────────────────
()  []  .                       // postfix
-x  !x                          // unary
x * y   x / y   x % y          // multiplicative
x + y   x - y                   // additive
x < y   x > y   x <= y  x >= y // relational
x == y  x != y                  // equality
x && y                          // logical AND
x || y                          // logical OR
x = y   x += y  x -= y  …      // assignment
```
