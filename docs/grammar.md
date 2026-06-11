# V Language — Complete Grammar (EBNF)

> Notation follows **ISO EBNF** (ISO/IEC 14977).
>
> | Meta-symbol | Meaning                             |
> |-------------|-------------------------------------|
> | `=`         | definition                          |
> | `;`         | end of rule                         |
> | `\|`         | alternation                         |
> | `,`         | concatenation                       |
> | `[ … ]`     | option (zero or one)                |
> | `{ … }`     | repetition (zero or more)           |
> | `( … )`     | grouping                            |
> | `" … "`     | terminal string                     |
> | `? … ?`     | special sequence (informal prose)   |
> | `(* … *)`   | comment                             |

---

## 1. Top-level structure

```ebnf
program        = { top_level_decl } , EOF ;

top_level_decl = function_decl
               | var_decl
               | import_decl
               ;

import_decl    = "import" , module_path , [ "as" , IDENTIFIER ] ;
module_path    = IDENTIFIER , { "." , IDENTIFIER } ;
```

---

## 2. Declarations

### 2.1 Function declaration

```ebnf
function_decl  = [ "pub" ] , "fn" , IDENTIFIER ,
                 "(" , [ param_list ] , ")" ,
                 [ "->" , type ] ,
                 block_stmt ;

param_list     = param , { "," , param } ;
param          = [ "mut" ] , IDENTIFIER , ":" , type ;
```

### 2.2 Variable declaration

```ebnf
var_decl       = ( "let" | "var" | "const" ) ,
                 IDENTIFIER ,
                 [ ":" , type ] ,
                 [ "=" , expression ] ;
```

> `let`   — immutable binding (value cannot be reassigned).  
> `var`   — mutable binding.  
> `const` — compile-time constant; initialiser is required.

---

## 3. Statements

```ebnf
statement      = block_stmt
               | var_decl
               | if_stmt
               | while_stmt
               | for_stmt
               | return_stmt
               | break_stmt
               | continue_stmt
               | expr_stmt
               ;

block_stmt     = "{" , { statement } , "}" ;

if_stmt        = "if" , expression , block_stmt ,
                 { "else" , "if" , expression , block_stmt } ,
                 [ "else" , block_stmt ] ;

while_stmt     = "while" , expression , block_stmt ;

for_stmt       = "for" , IDENTIFIER , "in" , expression , block_stmt ;

return_stmt    = "return" , [ expression ] ;

break_stmt     = "break" ;

continue_stmt  = "continue" ;

expr_stmt      = expression ;
```

> **No semicolons** — statements are terminated by the closing token of each
> construct (right brace, or end-of-line for single-expression statements).

---

## 4. Expressions

Expressions are listed from **lowest** to **highest** precedence.

```ebnf
expression     = assign_expr ;

(* Assignment — right-associative, lowest precedence *)
assign_expr    = logical_or_expr , [ assign_op , assign_expr ] ;

assign_op      = "=" | "+=" | "-=" | "*=" | "/=" | "%=" ;

(* Logical *)
logical_or_expr  = logical_and_expr , { "||" , logical_and_expr } ;
logical_and_expr = equality_expr    , { "&&" , equality_expr    } ;

(* Comparison *)
equality_expr  = relational_expr , { equality_op , relational_expr } ;
equality_op    = "==" | "!=" ;

relational_expr = additive_expr , { relational_op , additive_expr } ;
relational_op   = "<" | ">" | "<=" | ">=" ;

(* Arithmetic *)
additive_expr  = multiplicative_expr , { additive_op , multiplicative_expr } ;
additive_op    = "+" | "-" ;

multiplicative_expr = unary_expr , { multiplicative_op , unary_expr } ;
multiplicative_op   = "*" | "/" | "%" ;

(* Unary — right-to-left *)
unary_expr     = unary_op , unary_expr
               | postfix_expr ;
unary_op       = "-" | "!" ;

(* Postfix *)
postfix_expr   = primary_expr , { postfix_suffix } ;
postfix_suffix = call_suffix
               | index_suffix
               | member_suffix
               ;

call_suffix    = "(" , [ arg_list ] , ")" ;
index_suffix   = "[" , expression , "]" ;
member_suffix  = "." , IDENTIFIER ;

arg_list       = expression , { "," , expression } ;

(* Primary *)
primary_expr   = literal
               | IDENTIFIER
               | "(" , expression , ")"
               | cast_expr
               ;

cast_expr      = expression , "as" , type ;
```

---

## 5. Literals

```ebnf
literal        = INTEGER_LIT
               | FLOAT_LIT
               | STRING_LIT
               | BOOL_LIT
               | "nil"
               ;

BOOL_LIT       = "true" | "false" ;
```

---

## 6. Types

```ebnf
type           = named_type
               | pointer_type
               | slice_type
               | array_type
               | fn_type
               ;

named_type     = IDENTIFIER , { "::" , IDENTIFIER } ;

pointer_type   = "*" , [ "mut" ] , type ;

slice_type     = "[" , type , "]" ;

array_type     = "[" , type , ";" , expression , "]" ;

fn_type        = "fn" , "(" , [ type_list ] , ")" , [ "->" , type ] ;
type_list      = type , { "," , type } ;
```

### 6.1 Built-in types

| Type     | Description                  | Size     |
|----------|------------------------------|----------|
| `int`    | Platform-width signed int    | 64-bit   |
| `i8`     | 8-bit signed integer         | 8-bit    |
| `i16`    | 16-bit signed integer        | 16-bit   |
| `i32`    | 32-bit signed integer        | 32-bit   |
| `i64`    | 64-bit signed integer        | 64-bit   |
| `u8`     | 8-bit unsigned integer       | 8-bit    |
| `u32`    | 32-bit unsigned integer      | 32-bit   |
| `u64`    | 64-bit unsigned integer      | 64-bit   |
| `float`  | Platform-width float         | 64-bit   |
| `f32`    | 32-bit float                 | 32-bit   |
| `f64`    | 64-bit float                 | 64-bit   |
| `bool`   | Boolean value                | 1-bit    |
| `string` | UTF-8 string                 | fat ptr  |
| `char`   | Unicode scalar value (UTF-8) | 32-bit   |

---

## 7. Lexical tokens

```ebnf
IDENTIFIER     = ( LETTER | "_" ) , { LETTER | DIGIT | "_" } ;

INTEGER_LIT    = decimal_lit | hex_lit | binary_lit | octal_lit ;
decimal_lit    = NONZERO_DIGIT , { DIGIT } | "0" ;
hex_lit        = "0" , ( "x" | "X" ) , HEX_DIGIT , { HEX_DIGIT } ;
binary_lit     = "0" , ( "b" | "B" ) , BIT , { BIT } ;
octal_lit      = "0" , ( "o" | "O" ) , OCT_DIGIT , { OCT_DIGIT } ;

FLOAT_LIT      = DIGIT , { DIGIT } , "." , DIGIT , { DIGIT } ,
                 [ exponent ] ;
exponent       = ( "e" | "E" ) , [ "+" | "-" ] , DIGIT , { DIGIT } ;

STRING_LIT     = '"' , { string_char } , '"' ;
string_char    = ? any UTF-8 character except '"' and unescaped '\n' ?
               | escape_seq ;
escape_seq     = "\" , ( "n" | "t" | "r" | "\" | '"' | "0"
                       | "x" , HEX_DIGIT , HEX_DIGIT ) ;

LETTER         = "a" … "z" | "A" … "Z" ;
DIGIT          = "0" … "9" ;
NONZERO_DIGIT  = "1" … "9" ;
HEX_DIGIT      = DIGIT | "a" … "f" | "A" … "F" ;
OCT_DIGIT      = "0" … "7" ;
BIT            = "0" | "1" ;
EOF            = ? end of input ? ;
```

### 7.1 Keywords (reserved)

```
fn       let      var      const    return
if       else     while    for      in
break    continue pub      mut      self
import   module   struct   enum     interface
type     as       match    true     false   nil
```

### 7.2 Operators and punctuation

| Token       | Lexeme  | Token       | Lexeme |
|-------------|---------|-------------|--------|
| `PLUS`      | `+`     | `EQ`        | `=`    |
| `MINUS`     | `-`     | `PLUS_EQ`   | `+=`   |
| `STAR`      | `*`     | `MINUS_EQ`  | `-=`   |
| `SLASH`     | `/`     | `STAR_EQ`   | `*=`   |
| `PERCENT`   | `%`     | `SLASH_EQ`  | `/=`   |
| `EQEQ`      | `==`    | `PERCENT_EQ`| `%=`   |
| `BANG_EQ`   | `!=`    | `ARROW`     | `->`   |
| `LT`        | `<`     | `FAT_ARROW` | `=>`   |
| `GT`        | `>`     | `AND_AND`   | `&&`   |
| `LT_EQ`     | `<=`    | `OR_OR`     | `\|\|` |
| `GT_EQ`     | `>=`    | `BANG`      | `!`    |
| `LPAREN`    | `(`     | `RPAREN`    | `)`    |
| `LBRACE`    | `{`     | `RBRACE`    | `}`    |
| `LBRACKET`  | `[`     | `RBRACKET`  | `]`    |
| `COMMA`     | `,`     | `COLON`     | `:`    |
| `DOT`       | `.`     | `DOT_DOT`   | `..`   |
| `SEMICOLON` | `;`     | `COLON_COLON`| `::`  |

### 7.3 Comments

```ebnf
line_comment   = "//" , { ? any character ? } , NEWLINE ;
block_comment  = "/*" , { block_comment | ? any character ? } , "*/" ;
```

Block comments are nestable.

---

## 8. Operator precedence table

| Level | Operator(s)              | Associativity |
|-------|--------------------------|---------------|
| 1     | `= += -= *= /= %=`       | Right         |
| 2     | `\|\|`                   | Left          |
| 3     | `&&`                     | Left          |
| 4     | `== !=`                  | Left          |
| 5     | `< > <= >=`              | Left          |
| 6     | `+ -`                    | Left          |
| 7     | `* / %`                  | Left          |
| 8     | Unary `-` `!`            | Right (prefix)|
| 9     | Call `()` Index `[]` `.` | Left (postfix)|

---

## 9. Scoping rules

1. Every `{…}` block introduces a new scope.
2. Inner scopes shadow outer names.
3. Functions are pre-declared at module scope, enabling mutual recursion.
4. `let` bindings are immutable after initialisation.
5. `var` bindings are mutable.
6. `const` values must be compile-time evaluable.

---

## 10. Type inference

The compiler infers the type of a binding from its initialiser when no
explicit annotation is provided:

```v
let x = 42      // inferred: int
let f = 3.14    // inferred: float
let s = "hello" // inferred: string
let b = true    // inferred: bool
```

If neither annotation nor initialiser is present, the declaration is a
compile-time error.

---

## 11. Grammar summary (one-page reference)

```ebnf
program        = { top_level_decl } , EOF ;
top_level_decl = function_decl | var_decl | import_decl ;
import_decl    = "import" , module_path , [ "as" , IDENTIFIER ] ;
module_path    = IDENTIFIER , { "." , IDENTIFIER } ;
function_decl  = ["pub"] , "fn" , IDENTIFIER , "(" , [param_list] , ")" ,
                 ["->", type] , block_stmt ;
param_list     = param , {"," , param} ;
param          = ["mut"] , IDENTIFIER , ":" , type ;
var_decl       = ("let"|"var"|"const") , IDENTIFIER , [":"  , type] ,
                 ["=" , expression] ;
statement      = block_stmt | var_decl | if_stmt | while_stmt | for_stmt
               | return_stmt | break_stmt | continue_stmt | expr_stmt ;
block_stmt     = "{" , {statement} , "}" ;
if_stmt        = "if" , expression , block_stmt ,
                 {"else" "if" , expression , block_stmt} ,
                 ["else" , block_stmt] ;
while_stmt     = "while" , expression , block_stmt ;
for_stmt       = "for" , IDENTIFIER , "in" , expression , block_stmt ;
return_stmt    = "return" , [expression] ;
expression     = assign_expr ;
assign_expr    = logical_or_expr , [assign_op , assign_expr] ;
logical_or_expr  = logical_and_expr , {"||" , logical_and_expr} ;
logical_and_expr = equality_expr    , {"&&" , equality_expr} ;
equality_expr    = relational_expr  , {("=="|"!=") , relational_expr} ;
relational_expr  = additive_expr    , {("<"|">"|"<="|">=") , additive_expr} ;
additive_expr    = multiplicative_expr , {("+"|"-") , multiplicative_expr} ;
multiplicative_expr = unary_expr , {("*"|"/"|"%") , unary_expr} ;
unary_expr     = ("-"|"!") , unary_expr | postfix_expr ;
postfix_expr   = primary_expr , {call_suffix | index_suffix | member_suffix} ;
primary_expr   = literal | IDENTIFIER | "(" , expression , ")" ;
literal        = INTEGER_LIT | FLOAT_LIT | STRING_LIT | BOOL_LIT | "nil" ;
type           = named_type | pointer_type | slice_type | array_type | fn_type ;
```
