# V Language — Version 0.1 Implementation Plan

> **Compiler**: VCC (V Compiler Collection)  
> **Target**: V language v0.1 — complete specification, grammar, and C++20/CMake implementation plan  
> **Status legend**: ✅ implemented · 🔧 in progress · ❌ not yet started

---

## Table of Contents

1. [Language Rules Summary](#1-language-rules-summary)
2. [Formal Grammar (EBNF)](#2-formal-grammar-ebnf)
3. [Token Definitions](#3-token-definitions)
4. [AST Hierarchy](#4-ast-hierarchy)
5. [Recursive Descent Parser Design](#5-recursive-descent-parser-design)
6. [Semantic Analysis Design](#6-semantic-analysis-design)
7. [Symbol Table Design](#7-symbol-table-design)
8. [LLVM Backend Architecture](#8-llvm-backend-architecture)
9. [Complete Folder Structure](#9-complete-folder-structure)
10. [Implementation Status](#10-implementation-status)

---

## 1. Language Rules Summary

| # | Feature | Syntax |
|---|---------|--------|
| 1 | Function declaration | `fn name(args) { … }` |
| 2 | Forward references | Functions may call functions declared later in file |
| 3 | No semicolons | Statements are newline/brace delimited |
| 4 | Variable declaration | `let x = 10` |
| 5 | Conditional | `if(cond) { } else { }` |
| 6 | Loop | `vloop(cond) { }` |
| 7 | Operators | `+ - * / %`, `< > <= >= == !=`, `<< >> & \| ^ ~`, `&& \|\| !` |
| 8 | Comments | `# single line comment` |
| 9 | Built-in types | `int`, `float`, `bool`, `string` |
| 10 | Compiler | `vcc` |

---

## 2. Formal Grammar (EBNF)

> Notation: ISO EBNF (ISO/IEC 14977).  
> `{ x }` = zero or more,  `[ x ]` = optional,  `( a | b )` = alternation.

### 2.1 Program

```ebnf
program        = { top_level_decl } , EOF ;

top_level_decl = function_decl
               | var_decl
               ;
```

### 2.2 Function Declaration

```ebnf
function_decl  = "fn" , IDENTIFIER ,
                 "(" , [ param_list ] , ")" ,
                 block_stmt ;

param_list     = IDENTIFIER , { "," , IDENTIFIER } ;
```

> Functions may appear in any order; forward calls are resolved in a two-pass pre-scan.

### 2.3 Statements

```ebnf
statement      = var_decl
               | if_stmt
               | vloop_stmt
               | return_stmt
               | expr_stmt
               | block_stmt
               ;

block_stmt     = "{" , { statement } , "}" ;

var_decl       = "let" , IDENTIFIER , "=" , expression ;

if_stmt        = "if" , "(" , expression , ")" , block_stmt
                 [ "else" , ( if_stmt | block_stmt ) ] ;

vloop_stmt     = "vloop" , "(" , expression , ")" , block_stmt ;

return_stmt    = "return" , [ expression ] ;

expr_stmt      = expression ;
```

### 2.4 Expressions (precedence table)

| Prec | Operator(s) | Associativity |
|------|-------------|---------------|
| 1 (lowest) | `\|\|` | left |
| 2 | `&&` | left |
| 3 | `\|` | left |
| 4 | `^` | left |
| 5 | `&` | left |
| 6 | `== !=` | left |
| 7 | `< > <= >=` | left |
| 8 | `<< >>` | left |
| 9 | `+ -` | left |
| 10 | `* / %` | left |
| 11 (highest) | `! ~ - (unary)` | right |

```ebnf
expression     = assignment_expr ;

assignment_expr = ( IDENTIFIER , "=" , assignment_expr )
                | logical_or_expr ;

logical_or_expr  = logical_and_expr , { "||" , logical_and_expr } ;
logical_and_expr = bitwise_or_expr  , { "&&" , bitwise_or_expr  } ;
bitwise_or_expr  = bitwise_xor_expr , { "|"  , bitwise_xor_expr } ;
bitwise_xor_expr = bitwise_and_expr , { "^"  , bitwise_and_expr } ;
bitwise_and_expr = equality_expr    , { "&"  , equality_expr    } ;
equality_expr    = relational_expr  , { ( "==" | "!=" ) , relational_expr } ;
relational_expr  = shift_expr       , { ( "<" | ">" | "<=" | ">=" ) , shift_expr } ;
shift_expr       = additive_expr    , { ( "<<" | ">>" ) , additive_expr } ;
additive_expr    = mult_expr        , { ( "+" | "-" ) , mult_expr } ;
mult_expr        = unary_expr       , { ( "*" | "/" | "%" ) , unary_expr } ;

unary_expr     = ( "!" | "~" | "-" ) , unary_expr
               | postfix_expr ;

postfix_expr   = primary_expr , { call_suffix } ;

call_suffix    = "(" , [ arg_list ] , ")" ;

arg_list       = expression , { "," , expression } ;

primary_expr   = INTEGER_LITERAL
               | FLOAT_LITERAL
               | STRING_LITERAL
               | "true"
               | "false"
               | IDENTIFIER
               | "(" , expression , ")" ;
```

### 2.5 Types

```ebnf
type           = "int" | "float" | "bool" | "string" | IDENTIFIER ;
```

### 2.6 Lexical elements

```ebnf
(* Comments — skipped by lexer *)
line_comment   = "#" , { ANY_CHAR_EXCEPT_NEWLINE } , NEWLINE ;

IDENTIFIER     = LETTER , { LETTER | DIGIT | "_" } ;
INTEGER_LITERAL = DIGIT , { DIGIT } ;
FLOAT_LITERAL   = DIGIT , { DIGIT } , "." , DIGIT , { DIGIT } ;
STRING_LITERAL  = '"' , { STRING_CHAR } , '"' ;

LETTER         = "a" .. "z" | "A" .. "Z" | "_" ;
DIGIT          = "0" .. "9" ;
```

---

## 3. Token Definitions

### 3.1 Keyword tokens

| Token kind | Lexeme |
|------------|--------|
| `KwFn` | `fn` |
| `KwLet` | `let` |
| `KwReturn` | `return` |
| `KwIf` | `if` |
| `KwElse` | `else` |
| `KwVloop` | `vloop` ← **v0.1 specific** |
| `KwTrue` | `true` |
| `KwFalse` | `false` |
| `KwInt` | `int` |
| `KwFloat` | `float` |
| `KwBool` | `bool` |
| `KwString` | `string` |

### 3.2 Operator tokens

| Token kind | Lexeme | Notes |
|------------|--------|-------|
| `Plus` | `+` | |
| `Minus` | `-` | |
| `Star` | `*` | |
| `Slash` | `/` | |
| `Percent` | `%` | |
| `EqEq` | `==` | |
| `NotEq` | `!=` | |
| `Lt` | `<` | |
| `Gt` | `>` | |
| `LtEq` | `<=` | |
| `GtEq` | `>=` | |
| `Shl` | `<<` | bitwise shift left |
| `Shr` | `>>` | bitwise shift right |
| `Amp` | `&` | bitwise AND |
| `Pipe` | `\|` | bitwise OR |
| `Caret` | `^` | bitwise XOR |
| `Tilde` | `~` | bitwise NOT (unary) |
| `AmpAmp` | `&&` | logical AND |
| `PipePipe` | `\|\|` | logical OR |
| `Bang` | `!` | logical NOT |
| `Eq` | `=` | assignment |

### 3.3 Punctuation tokens

| Token kind | Lexeme |
|------------|--------|
| `LParen` | `(` |
| `RParen` | `)` |
| `LBrace` | `{` |
| `RBrace` | `}` |
| `Comma` | `,` |

### 3.4 Special tokens

| Token kind | Meaning |
|------------|---------|
| `IntLiteral` | Integer constant |
| `FloatLiteral` | Floating-point constant |
| `StringLiteral` | Double-quoted string |
| `Identifier` | User-defined name |
| `Eof` | End of input |

### 3.5 Comment handling

`#` starts a line comment — the lexer skips every character up to (not including) the next `\n`. Comments produce no tokens; they are invisible to the parser.

---

## 4. AST Hierarchy

```
Node (abstract)
├── Decl (abstract)
│   ├── FunctionDecl      fn name(params) block
│   └── VarDecl           let name = expr
│
├── Stmt (abstract)
│   ├── BlockStmt         { stmt* }
│   ├── ExprStmt          expr
│   ├── DeclStmt          wraps VarDecl
│   ├── IfStmt            if(cond) then [else]
│   ├── VloopStmt         vloop(cond) body       ← v0.1
│   ├── ReturnStmt        return [expr]
│   ├── BreakStmt         break
│   └── ContinueStmt      continue
│
└── Expr (abstract)
    ├── LiteralExpr       int / float / bool / string constant
    ├── IdentifierExpr    variable reference
    ├── BinaryExpr        lhs op rhs
    ├── UnaryExpr         op operand
    ├── AssignExpr        lhs = rhs
    └── CallExpr          callee(args*)
```

### 4.1 Key node interfaces

```cpp
// Every node carries source location
struct Node {
    virtual ~Node() = default;
    virtual std::string_view nodeName() const noexcept = 0;
    virtual void accept(ASTVisitor&) = 0;
    common::SourceLocation loc;
};

// VloopStmt — same shape as WhileStmt
class VloopStmt : public Stmt {
public:
    VloopStmt(std::unique_ptr<Expr> condition,
              std::unique_ptr<BlockStmt> body,
              common::SourceLocation loc);
    Expr&      condition() const;
    BlockStmt& body()      const;
    std::string_view nodeName() const noexcept override { return "VloopStmt"; }
    void accept(ASTVisitor& v) override;
};
```

### 4.2 Visitor interface additions

```cpp
// ASTVisitor.h — add alongside visitWhileStmt:
virtual void visitVloopStmt(VloopStmt& node) = 0;
```

---

## 5. Recursive Descent Parser Design

### 5.1 Class interface

```cpp
class Parser {
public:
    explicit Parser(std::vector<common::Token> tokens,
                    common::DiagnosticEngine& diag);

    std::unique_ptr<ast::Program> parseProgram();

private:
    // Statements
    std::unique_ptr<ast::Stmt>        parseStatement();
    std::unique_ptr<ast::BlockStmt>   parseBlock();
    std::unique_ptr<ast::VarDecl>     parseVarDecl();
    std::unique_ptr<ast::IfStmt>      parseIfStmt();
    std::unique_ptr<ast::VloopStmt>   parseVloopStmt();   // ← v0.1
    std::unique_ptr<ast::ReturnStmt>  parseReturnStmt();
    std::unique_ptr<ast::FunctionDecl> parseFunctionDecl();

    // Expressions (Pratt / recursive descent)
    std::unique_ptr<ast::Expr> parseExpression();
    std::unique_ptr<ast::Expr> parseAssignment();
    std::unique_ptr<ast::Expr> parseBinary(int minPrec);
    std::unique_ptr<ast::Expr> parseUnary();
    std::unique_ptr<ast::Expr> parsePostfix();
    std::unique_ptr<ast::Expr> parsePrimary();

    // Token helpers
    common::Token& peek(int offset = 0);
    common::Token  advance();
    bool           check(common::TokenKind kind);
    bool           match(common::TokenKind kind);
    common::Token  expect(common::TokenKind kind, std::string_view msg);
};
```

### 5.2 vloop parsing

```cpp
std::unique_ptr<VloopStmt> Parser::parseVloopStmt() {
    auto loc = peek().location;
    expect(TokenKind::KwVloop, "expected 'vloop'");
    expect(TokenKind::LParen,  "expected '(' after 'vloop'");
    auto cond = parseExpression();
    expect(TokenKind::RParen,  "expected ')' after vloop condition");
    auto body = parseBlock();
    return std::make_unique<VloopStmt>(std::move(cond), std::move(body), loc);
}
```

### 5.3 Operator precedence table (Pratt)

```cpp
static int getBinaryPrec(TokenKind k) {
    switch (k) {
    case TokenKind::PipePipe:  return 1;
    case TokenKind::AmpAmp:    return 2;
    case TokenKind::Pipe:      return 3;
    case TokenKind::Caret:     return 4;
    case TokenKind::Amp:       return 5;
    case TokenKind::EqEq:
    case TokenKind::NotEq:     return 6;
    case TokenKind::Lt: case TokenKind::Gt:
    case TokenKind::LtEq: case TokenKind::GtEq: return 7;
    case TokenKind::Shl:
    case TokenKind::Shr:       return 8;
    case TokenKind::Plus:
    case TokenKind::Minus:     return 9;
    case TokenKind::Star:
    case TokenKind::Slash:
    case TokenKind::Percent:   return 10;
    default:                   return -1;
    }
}
```

### 5.4 Two-pass function pre-scan (forward references)

```
Pass 1:  walk top-level tokens; collect all `fn NAME(` signatures → seed symbol table
Pass 2:  full parse — CallExpr resolution can now find any function defined later
```

---

## 6. Semantic Analysis Design

### 6.1 Checks performed

| # | Check | Error |
|---|-------|-------|
| 1 | Variable declared before use | `error: undefined variable 'x'` |
| 2 | Duplicate variable declarations | `error: 'x' already declared in this scope` |
| 3 | Type checking | `error: type mismatch: expected int, found bool` |
| 4 | Function existence | `error: undefined function 'foo'` |
| 5 | Return statement validation | `error: return outside function` |
| 6 | Scope validation | `error: 'x' not accessible in this scope` |

### 6.2 Three-pass pipeline

```
Pass 1 — FunctionCollector:   register all function signatures (enables forward calls)
Pass 2 — SemanticAnalyzer:    scope/variable/call checks via visitor
Pass 3 — TypeChecker:         type inference and compatibility checks
```

### 6.3 vloop semantic check

`VloopStmt` is treated identically to `WhileStmt`: the condition is checked for boolean-compatibility and the body is analyzed in a new inner scope.

```cpp
void SemanticAnalyzer::visitVloopStmt(VloopStmt& node) {
    node.condition().accept(*this);
    checkBoolCompatible(node.condition(), "vloop condition");
    scopeStack_.enterScope();
    node.body().accept(*this);
    scopeStack_.exitScope();
}
```

---

## 7. Symbol Table Design

### 7.1 Symbol kinds

```cpp
enum class SymbolKind { Variable, Function, Parameter };

struct Symbol {
    std::string      name;
    SymbolKind       kind;
    std::string      type;          // "int" | "float" | "bool" | "string"
    bool             isMutable{true};
    SourceLocation   declLoc;

    // Function-specific
    std::vector<std::string> paramTypes;
    std::string              returnType;
};
```

### 7.2 Scope

```cpp
class Scope {
public:
    void  define(Symbol sym);
    Symbol* lookup(std::string_view name);          // current scope only
    Symbol* lookupAll(std::string_view name);        // walk parent chain
    void  enterScope();
    void  exitScope();
private:
    std::vector<std::unordered_map<std::string, Symbol>> stack_;
};
```

### 7.3 Scope nesting model

```
GlobalScope
  └── FunctionScope (one per fn)
        └── BlockScope (one per { })
              └── BlockScope (nested blocks — if/vloop bodies)
```

---

## 8. LLVM Backend Architecture

### 8.1 Pipeline

```
AST
 └─► IRGen (VCC IR)          visitor-based AST → VCC IR lowering
      └─► LLVMCodeGenerator  VCC IR → LLVM IR (via LLVM 18 C++ API)
           └─► output.ll     bitcode or textual IR file
```

### 8.2 VCC IR instruction set

| Instruction | Syntax | Notes |
|-------------|--------|-------|
| `alloca` | `x = alloca` | stack slot for variable |
| `param N` | `t1 = param 0` | retrieve Nth function argument |
| `load` | `t2 = load x` | read variable slot |
| `store` | `store t2 x` | write variable slot |
| `add/sub/mul/div/mod` | `t3 = add t1 t2` | arithmetic |
| `shl/shr` | `t4 = shl t1 t2` | bitwise shift |
| `band/bor/bxor/bnot` | `t5 = band t1 t2` | bitwise ops |
| `and/or/not` | `t6 = and t1 t2` | logical ops |
| `eq/ne/lt/gt/le/ge` | `t7 = eq t1 t2` | comparisons |
| `branch` | `branch label` | unconditional jump |
| `condbranch` | `condbranch t7 true,false` | conditional jump |
| `call` | `t8 = call foo t1 t2` | function call |
| `ret` | `ret t8` | return value |

### 8.3 vloop IR lowering pattern

```
vloop(cond) { body }
──────────────────────────────────────────
loop_header:
    t1 = <evaluate cond>
    condbranch t1 loop_body, loop_exit

loop_body:
    <lower body statements>
    branch loop_header

loop_exit:
    <continue here>
```

### 8.4 LLVMCodeGenerator responsibilities

```cpp
class LLVMCodeGenerator {
public:
    bool lower(ir::IRModule& mod);
    bool emitIR(ir::IRModule& mod, const std::string& path);
private:
    std::unique_ptr<llvm::LLVMContext> ctx_;
    // Per-function: maps VCC RegID → llvm::Value*
    // Handles Param, Alloca, Load, Store, BinOp, CondBranch, Call, Ret
};
```

### 8.5 LLVM type mapping

| V type | LLVM type |
|--------|-----------|
| `int` | `i64` |
| `float` | `double` |
| `bool` | `i1` |
| `string` | `i8*` |

---

## 9. Complete Folder Structure

```
vcc/
├── CMakeLists.txt                 # Root CMake; VCC_ENABLE_LLVM option
│
├── include/
│   └── vcc/
│       ├── common/
│       │   ├── Token.h            # TokenKind enum (KwVloop ← add)
│       │   ├── SourceLocation.h
│       │   ├── Diagnostics.h      # DiagnosticEngine
│       │   └── CompilerContext.h  # CompilerOptions
│       │
│       ├── lexer/
│       │   └── Lexer.h            # skipLineComment handles '#'
│       │
│       ├── ast/
│       │   ├── ASTNode.h          # Node, Stmt, Expr base classes
│       │   ├── ASTVisitor.h       # visitVloopStmt ← add
│       │   ├── Declarations.h     # FunctionDecl, VarDecl
│       │   ├── Statements.h       # VloopStmt ← add
│       │   ├── Expressions.h      # BinaryExpr, UnaryExpr, CallExpr …
│       │   ├── Types.h
│       │   ├── AST.h              # convenience include-all
│       │   └── ASTDumper.h
│       │
│       ├── parser/
│       │   └── Parser.h           # parseVloopStmt ← add
│       │
│       ├── semantic/
│       │   ├── SemanticAnalyzer.h # visitVloopStmt ← add
│       │   ├── TypeChecker.h
│       │   ├── SymbolTable.h
│       │   └── Scope.h
│       │
│       ├── ir/
│       │   ├── IRInstruction.h    # Opcode enum (Shl,Shr,Band,Bor,Bxor,Bnot)
│       │   ├── IRModule.h
│       │   ├── IRBuilder.h
│       │   ├── IRGen.h            # visitVloopStmt ← add
│       │   └── IRPrinter.h
│       │
│       └── codegen/
│           └── LLVMCodeGenerator.h
│
├── src/
│   ├── common/
│   │   ├── Token.cpp              # keywords() map: "vloop" → KwVloop ← add
│   │   └── Diagnostics.cpp
│   │
│   ├── lexer/
│   │   └── Lexer.cpp              # '#' triggers skipLineComment ← fix
│   │
│   ├── ast/
│   │   ├── ASTNode.cpp
│   │   ├── ASTDumper.cpp          # VloopStmt case ← add
│   │   └── Statements.cpp         # VloopStmt implementation ← add
│   │
│   ├── parser/
│   │   └── Parser.cpp             # parseVloopStmt(), KwVloop dispatch ← add
│   │
│   ├── semantic/
│   │   ├── SemanticAnalyzer.cpp   # visitVloopStmt ← add
│   │   ├── TypeChecker.cpp
│   │   └── SymbolTable.cpp
│   │
│   ├── ir/
│   │   ├── IRInstruction.cpp
│   │   ├── IRModule.cpp
│   │   ├── IRBuilder.cpp
│   │   ├── IRGen.cpp              # visitVloopStmt ← add
│   │   └── IRPrinter.cpp
│   │
│   ├── codegen/
│   │   ├── LLVMCodeGenerator.cpp
│   │   └── LLVMBackend.cpp
│   │
│   └── driver/
│       ├── main.cpp               # CLI: --tokens --ast --ir --emit-llvm
│       └── Compiler.cpp           # 6-stage pipeline
│
├── tests/
│   ├── test_lexer.cpp             # TokenKind::KwVloop, '#' comment tests
│   ├── test_parser.cpp            # VloopStmt parse tests
│   ├── test_semantic.cpp          # vloop scope/type tests
│   └── test_ir.cpp                # vloop IR lowering tests
│
├── examples/
│   ├── hello.v
│   ├── functions.v
│   └── vloop_example.v            # demonstrates vloop
│
└── docs/
    ├── grammar.md
    ├── language_spec.md
    ├── syntax.md
    ├── architecture.md
    ├── SELF_HOSTING_ROADMAP.md
    └── V_LANGUAGE_V01_PLAN.md     # this file
```

---

## 10. Implementation Status

### 10.1 What is already done ✅

| Component | Status | Notes |
|-----------|--------|-------|
| Lexer | ✅ | All operators incl. `<<`, `>>`, `&`, `\|`, `^`, `~`; `//` comments |
| Token set | ✅ | All v0.1 tokens except `KwVloop` |
| `#` comment skip | ❌ | `#` tokenized as `Hash`; not yet skipped in `skipWhitespaceAndComments` |
| AST hierarchy | ✅ | All nodes; `WhileStmt` exists |
| `VloopStmt` node | ❌ | Not present; must be added alongside `WhileStmt` |
| Parser | ✅ | Full Pratt parser; all statements |
| `parseVloopStmt` | ❌ | Not present; triggered by `KwVloop` |
| Semantic analysis | ✅ | All 6 checks; `WhileStmt` handled |
| `visitVloopStmt` | ❌ | Not in SemanticAnalyzer/TypeChecker/ASTDumper |
| Symbol table | ✅ | Scopes, variables, functions |
| VCC IR | ✅ | Full opcode set; `IRBuilder`; `IRGen` |
| `vloop` IR lowering | ❌ | `visitVloopStmt` not in `IRGen` |
| LLVM backend | ✅ | LLVM 18; `output.ll` emission |
| Compiler driver | ✅ | `--tokens --ast --ir --emit-llvm` |
| Diagnostics | ✅ | ANSI colors; `error:` / `warning:` / `note:` |

### 10.2 v0.1 remaining work (4 items)

```
1. src/lexer/Lexer.cpp
   skipWhitespaceAndComments(): add  else if (ch == '#') skipLineComment();

2. include/vcc/common/Token.h
   Add:  KwVloop,   ///< vloop

3. src/common/Token.cpp  (keywords map)
   Add:  {"vloop", TokenKind::KwVloop},

4. AST → Parser → Semantic → IRGen → ASTDumper
   Add VloopStmt everywhere WhileStmt appears (identical logic)
```

### 10.3 Compiler commands (v0.1)

```bash
vcc file.v                  # full pipeline → output.ll
vcc file.v --tokens         # lex only, print token table
vcc file.v --ast            # parse + dump AST
vcc file.v --ir             # gen VCC IR, print
vcc file.v --emit-llvm      # emit LLVM IR to output.ll
vcc file.v -o prog          # emit + name output file
vcc file.v -v               # verbose (show pipeline stages)
```

---

*End of V Language v0.1 Implementation Plan*
