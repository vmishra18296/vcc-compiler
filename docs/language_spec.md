# V Language Specification (Draft)

> **Status**: Work in progress – Phase 1 grammar.

---

## 1. Lexical structure

### 1.1 Source encoding

V source files are UTF-8 encoded.  Line endings may be `LF` or `CRLF`.

### 1.2 Comments

```v
// single-line comment

/* block comment
   may be /* nested */ */
```

### 1.3 Identifiers

```
Identifier := [a-zA-Z_][a-zA-Z0-9_]*
```

Unicode identifiers (XID_Start / XID_Continue) will be supported in a future revision.

### 1.4 Keywords

```
fn  let  var  const  if  else  while  for  in  return
import  module  struct  interface  enum  type
true  false  nil  pub  mut  self  as  break  continue  match
```

### 1.5 Integer literals

| Form      | Example    |
|-----------|------------|
| Decimal   | `42`       |
| Hex       | `0xFF`     |
| Binary    | `0b1010`   |
| Octal     | `0o77`     |

### 1.6 Float literals

```
42.0    3.14    1.0e9    6.022e-23
```

### 1.7 String literals

```v
"hello, world"
"escape: \n \t \r \\ \""
```

### 1.8 Character literals

```v
'a'    '\n'    '\x41'
```

---

## 2. Types

### 2.1 Primitive types

| Type    | Description            |
|---------|------------------------|
| `i8`    | 8-bit signed integer   |
| `i16`   | 16-bit signed integer  |
| `i32`   | 32-bit signed integer  |
| `i64`   | 64-bit signed integer  |
| `u8`    | 8-bit unsigned integer |
| `u16`   | 16-bit unsigned integer|
| `u32`   | 32-bit unsigned integer|
| `u64`   | 64-bit unsigned integer|
| `f32`   | 32-bit float           |
| `f64`   | 64-bit float           |
| `bool`  | Boolean                |
| `str`   | UTF-8 string slice     |
| `char`  | Unicode scalar value   |

### 2.2 Compound types

```v
[T]        // slice of T
[T; N]     // array of N elements of type T
*T         // immutable pointer to T
*mut T     // mutable pointer to T
fn(T) -> R // function type
```

---

## 3. Declarations

### 3.1 Module

```v
module name
```

### 3.2 Import

```v
import std.io
import math as m
```

### 3.3 Function

```v
fn name(param: Type, …) -> ReturnType {
    body
}
pub fn public_fn() {}
```

### 3.4 Struct

```v
struct Point {
    pub x: f64,
    pub y: f64,
}
```

### 3.5 Enum

```v
enum Color {
    Red,
    Green,
    Blue,
    Custom(u8, u8, u8),
}
```

### 3.6 Variable bindings

```v
let name = expr           // immutable inferred
let name: Type = expr     // immutable explicit type
var name = expr           // mutable
var name: Type = expr
const NAME: Type = expr   // compile-time constant
```

### 3.7 Type alias

```v
type Meters = f64
```

---

## 4. Statements

```v
if condition { … } else { … }

while condition { … }

for variable in iterable { … }

return expr

break
continue
```

---

## 5. Expressions

### 5.1 Operator precedence (high to low)

| Precedence | Operators                       |
|------------|---------------------------------|
| 10         | `*`  `/`  `%`                   |
| 9          | `+`  `-`                        |
| 8          | `<<`  `>>`                      |
| 7          | `<`  `>`  `<=`  `>=`            |
| 6          | `==`  `!=`                      |
| 5          | `&`                             |
| 4          | `^`                             |
| 3          | `\|`                            |
| 2          | `&&`                            |
| 1          | `\|\|`                          |
| 0          | `=`  `+=`  `-=` … (assignment)  |

All binary operators are left-associative except assignment (right-associative).

### 5.2 Unary operators

```v
-x    !x    ~x    *x    &x
```

### 5.3 Cast

```v
expr as TargetType
```

---

## 6. Future language features (planned)

- Generics: `fn id<T>(x: T) -> T`
- Match expressions: `match val { pattern => expr, … }`
- Closures: `|x| x + 1`
- Interfaces / traits
- Error handling: `Result<T, E>` and `?` operator
- Modules and visibility rules
