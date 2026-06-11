# VCC Self-Hosting Roadmap

**Goal:** Rewrite VCC in V so that the V compiler compiles itself, retiring the
C++ implementation as the production build tool.

---

## Terminology

| Term | Meaning |
|---|---|
| **VCC (C++)** | The current C++20 compiler — the bootstrap host |
| **VCC (V)** | The future self-hosted compiler written in V |
| **Stage N** | A compiler binary produced by a specific previous stage |
| **Bootstrap** | The process of a compiler compiling its own source |

---

## Current State

```
V source code
     │  compiled by
     ▼
  VCC (C++)  ──→  LLVM IR  ──→  native binary
```

The C++ VCC is ~4,300 lines across six pipeline modules:

```
vcc_common   – tokens, diagnostics, source locations
vcc_lexer    – character scanner → token stream
vcc_ast      – AST node hierarchy (Decl / Stmt / Expr / Type)
vcc_parser   – Pratt recursive-descent → AST
vcc_semantic – two-pass name resolution + type checking
vcc_ir       – 3-address VCC IR + IRBuilder + IRPrinter
vcc_codegen  – LLVM 18 lowering → .ll text
vcc_driver   – 6-stage pipeline orchestration
```

V language features today:
- Scalar types: `i64`, `f64`, `bool`, `str`
- Variables: `let` (immutable), `var` (mutable)
- Control flow: `if`/`else`, `while`, `for … in`
- Functions with typed parameters and return types
- Arithmetic, bitwise, logical, and comparison operators
- String concatenation
- Modules and `import`

**Gap:** V has no structs, no arrays, no generics, no interfaces, no error
handling types, and no standard library. These are all required to write a
compiler in it.

---

## Phases at a Glance

```
Phase A  Language Completeness  ──  V gains the features needed to write a compiler
Phase B  Standard Library       ──  collections, I/O, strings, formatting
Phase C  Bootstrap Infrastructure──  package system, build tooling, test runner
Phase D  VCC-in-V Prototype     ──  port each C++ module to V (hosted by VCC C++)
Phase E  Self-Hosted Bootstrap  ──  VCC-in-V compiles VCC-in-V; C++ host retired
```

Total estimated scope: 12–18 months for a single focused engineer.

---

## Phase A — Language Completeness

VCC (V) needs every language feature listed below. Each is a prerequisite for
writing one or more compiler modules.

### A1 — Struct Types  *(blocks: all modules)*

```v
struct Token {
    kind:    TokenKind
    lexeme:  str
    line:    i64
    column:  i64
}
```

Required for: `Token`, `SourceLocation`, `IRInstruction`, `Symbol`, every
AST node, `IRBasicBlock`, `CompilerOptions`.

**Compiler changes:**
- Add struct declaration syntax to parser
- Add struct type to AST (`StructTypeNode`, `StructDecl`)
- Extend semantic analysis: field resolution, struct literal type-checking
- IRGen: lower struct literals to a sequence of alloca + field stores;
  lower field access (`expr.field`) to a GEP-style offset load

### A2 — Array / Slice Types  *(blocks: Lexer, Parser, IRModule)*

```v
let tokens: []Token = []
tokens.push(t)
let first = tokens[0]
let n     = tokens.len()
```

Required for: token stream (`[]Token`), AST child lists (`[]unique Expr`),
`IRBasicBlock.instructions`, `IRFunction.blocks`.

**Compiler changes:**
- Array literal syntax + indexing + `.len()` / `.push()` / `.pop()`
- Slice type `[]T` in the type system
- IRGen: `alloca [N x i64]` + GEP for fixed arrays;
  fat-pointer `{ptr, len, cap}` struct for dynamic slices

### A3 — Enums with Associated Data  *(blocks: Token, Opcode, AST kind tags)*

```v
enum Opcode {
    Add
    Sub
    Call(callee: str)
    Ret(hasValue: bool)
}
```

Required for: `TokenKind`, `Opcode`, `Operand.Kind`, AST node type tags,
`DiagnosticSeverity`.

**Compiler changes:**
- Enum declaration + variant construction in parser and semantic passes
- Tagged-union IRGen strategy: `{tag: i64, payload: [N x i8]}` or per-variant structs

### A4 — Interfaces / Traits  *(blocks: CodeGen, ASTVisitor)*

```v
interface ASTVisitor {
    visit_fn_decl(node: FunctionDecl)
    visit_var_decl(node: VarDecl)
    // …
}

fn walk(node: ASTNode, v: ASTVisitor) { … }
```

Required for: the visitor pattern used in semantic analysis, IRGen, and
ASTDumper; the abstract `CodeGen` interface.

**Compiler changes:**
- Interface declaration; method dispatch via vtable
- Implement `interface` as a fat pointer `{data_ptr, vtable_ptr}` in IRGen

### A5 — Option and Result Types  *(blocks: Parser error recovery)*

```v
fn find_symbol(name: str) -> Option[Symbol] { … }
fn parse_expr() -> Result[Expr, ParseError]  { … }

if let sym = table.get(name) {
    // sym is Symbol here
}
```

Required for: parser error recovery, symbol lookup returns, file I/O errors.

**Compiler changes:**
- Built-in generic `Option[T]` and `Result[T, E]` (or language-level
  `?`-unwrap operator)
- `if let` / `while let` destructuring syntax
- IRGen: lower to a two-field struct `{tag: i8, value: T}`

### A6 — Generics (Parametric Types)  *(blocks: collections)*

```v
struct Vec[T] { … }
struct HashMap[K, V] { … }

fn map[T, U](xs: []T, f: fn(T) -> U) -> []U { … }
```

Required for: `Vec[Token]`, `HashMap[str, Symbol]`, `Vec[unique[ASTNode]]`.
Without generics, every container must be duplicated per element type.

**Compiler changes:**
- Generic type parameter syntax on struct and fn declarations
- Monomorphisation: at the call site, instantiate a concrete copy with
  concrete type arguments (simplest strategy; avoids boxing overhead)
- IRGen: each monomorphised instance is an independent LLVM function/type

### A7 — Ownership / Unique Pointers  *(blocks: AST node ownership)*

The C++ compiler uses `std::unique_ptr<ASTNode>` for exclusive ownership of
AST nodes. V needs an equivalent that prevents use-after-free without a GC.

Recommended strategy: **arena allocation** with lifetime regions.

```v
// Compiler-managed arena; all nodes freed at region end
let arena = Arena.new()
let node  = arena.alloc(FunctionDecl { … })   // node: &FunctionDecl
```

Alternatively, add a `unique[T]` built-in:
```v
let decl: unique[FunctionDecl] = unique(FunctionDecl { … })
```

**Decision:** Choose one memory model before Phase D begins. Arena allocation
is simpler to implement and fits a compiler's "allocate everything, free at
the end" lifetime pattern.

### A8 — Pattern Matching  *(blocks: semantic analysis, IRGen switch)*

```v
match tok.kind {
    TokenKind.Fn      => parse_fn_decl()
    TokenKind.Let     => parse_var_decl()
    TokenKind.If      => parse_if_stmt()
    _                 => parse_expr_stmt()
}
```

Required for: every `switch (tok.kind)` in the parser and every
`switch (instr.opcode)` in IRGen / IRPrinter.

**Compiler changes:**
- `match` expression with exhaustiveness checking
- IRGen: lower to LLVM `switch i64` or a chain of `icmp` + `br`

---

## Phase B — Standard Library

### B1 — `std.string`

```v
import std.string

let s = StringBuilder.new()
s.append("hello ")
s.append(name)
let result = s.to_str()

let parts = "a,b,c".split(",")
let upper = "hello".to_upper()
```

Required for: diagnostic message construction, source text manipulation,
token lexeme storage, IRPrinter output.

Key APIs: `StringBuilder`, `split`, `trim`, `starts_with`, `ends_with`,
`to_upper`, `to_lower`, `char_at`, `byte_slice`, `to_i64`, `to_f64`.

### B2 — `std.collections`

```v
import std.collections

let tokens: Vec[Token]       = Vec.new()
let table:  HashMap[str,Symbol] = HashMap.new()
let seen:   HashSet[str]     = HashSet.new()
```

Required for: token stream, symbol table, basic block list, instruction list,
register → value map in LLVMCodeGenerator.

### B3 — `std.io`

```v
import std.io

let text = io.read_file("main.v")?      // Result[str, io.Error]
io.write_file("output.ll", llvm_ir)?
io.println("vcc: wrote output.ll")
io.eprintln("error: " + msg)
```

Required for: source file reading in the driver, writing `.ll` output,
diagnostic printing to stderr.

### B4 — `std.os`

```v
import std.os

let args = os.args()             // []str
let cwd  = os.cwd()             // str
let env  = os.env("HOME")?      // Option[str]
```

Required for: `main.v` argument parsing, path resolution.

### B5 — `std.fmt`

```v
import std.fmt

let msg = fmt.format("{}:{}: error: {}", file, line, text)
let hex = fmt.hex(0xDEAD_BEEF)
```

Required for: diagnostic renderer (the `file:line:col: error:` format).

### B6 — `std.testing`

```v
import std.testing

fn test_lexer_integer() {
    let tokens = lex("42")
    testing.expect_eq(tokens[0].kind, TokenKind.Integer)
}
```

Required for: the 200+ unit tests currently in GoogleTest must be rewritten
in V's native test framework.

---

## Phase C — Bootstrap Infrastructure

### C1 — Multi-file Module System

VCC (V) will be split across ~20 source files mirroring the current C++ layout.
V must support compiling a directory of `.v` files as a named module with
proper name resolution across files.

```
vcc/
├── lexer/
│   ├── lexer.v       // module vcc.lexer
│   └── token.v
├── parser/
│   └── parser.v      // module vcc.parser
…
```

### C2 — `vcc.v` Build Tool (written in V)

Once `std.os` and `std.io` exist, write a minimal `vcc.v` build tool that:
1. Scans source directories for `.v` files
2. Determines a topological compilation order from `import` statements
3. Invokes `vcc` (C++ or V) on each module
4. Links object files with `lld` or system linker

This is the analogue of the current CMakeLists.txt but written in V itself.

### C3 — Test Runner in V

Port the CMake + GoogleTest harness to a pure-V test runner:
1. Discovers test functions (`fn test_*()`) in `.v` files
2. Runs each test in an isolated environment
3. Reports pass / fail with diff output on mismatch

### C4 — LLVM C API Bindings

VCC (V) must lower to LLVM IR. Two strategies:

**Option A — LLVM C API via V FFI (recommended for Phase D)**
```v
// vcc/codegen/llvm_ffi.v
extern "C" {
    fn LLVMContextCreate() -> LLVMContextRef
    fn LLVMModuleCreateWithName(name: str) -> LLVMModuleRef
    fn LLVMBuildAdd(b: LLVMBuilderRef, lhs: LLVMValueRef, rhs: LLVMValueRef, name: str) -> LLVMValueRef
    // …
}
```
VCC (C++) already calls LLVM C++ APIs; V will call the stable LLVM C API
(`llvm-c/Core.h`) via an FFI layer.

**Option B — Emit textual LLVM IR directly**
VCC (V) produces a `.ll` string and calls `llvm-as` / `llc` as subprocesses.
Simpler to implement first; eliminates the need for FFI.
Use Option B for the initial prototype; migrate to Option A once FFI is stable.

---

## Phase D — VCC-in-V Prototype

Port each C++ module to V in dependency order. At this stage, VCC (C++) still
compiles the V source. All existing tests must pass after each port.

### Porting Order (dependency graph)

```
Step 1  vcc.common    ← SourceLocation, Token, Diagnostic, CompilerContext
Step 2  vcc.lexer     ← depends on vcc.common
Step 3  vcc.ast       ← depends on vcc.common
Step 4  vcc.parser    ← depends on vcc.ast, vcc.lexer
Step 5  vcc.semantic  ← depends on vcc.ast, vcc.parser
Step 6  vcc.ir        ← depends on vcc.ast, vcc.common
         IRInstruction, IRModule, IRBuilder, IRPrinter, IRGen
Step 7  vcc.codegen   ← depends on vcc.ir (LLVMCodeGenerator via C API or text)
Step 8  vcc.driver    ← depends on all above; the compiler entry point
```

### Step 1 — `vcc.common`

| C++ entity | V equivalent |
|---|---|
| `enum class TokenKind` | `enum TokenKind { Fn, Let, … }` |
| `struct Token` | `struct Token { kind, lexeme, line, column }` |
| `struct SourceLocation` | `struct SourceLocation { file, line, col }` |
| `struct Diagnostic` | `struct Diagnostic { severity, loc, message }` |
| `class CompilerContext` | `struct CompilerContext { opts, diags, sources }` |
| `DiagnosticsEngine` | methods on `CompilerContext` |

**Prerequisite language features:** A1 (structs), A3 (enums), B1 (strings),
B3 (file I/O for source registration).

### Step 2 — `vcc.lexer`

The `Lexer` is ~340 lines of C++. In V it becomes a struct with methods:

```v
struct Lexer {
    source: str
    pos:    i64
    line:   i64
    col:    i64
}

fn (l: &mut Lexer) tokenize() -> Vec[Token] { … }
fn (l: &mut Lexer) next_token() -> Token    { … }
```

The keyword map becomes a `HashMap[str, TokenKind]` (requires B2).

**Prerequisite language features:** A1, A2 (arrays for token list), A3,
B1 (string indexing), B2 (HashMap for keyword map).

### Step 3 — `vcc.ast`

The C++ AST uses `std::unique_ptr` and virtual dispatch. In V:

```v
// Sealed enum of all node kinds — exhaustive matching in visitors
enum ASTNode {
    FunctionDecl(FunctionDeclData)
    VarDecl(VarDeclData)
    BlockStmt(BlockStmtData)
    BinaryExpr(BinaryExprData)
    // …
}

struct FunctionDeclData {
    name:   str
    params: Vec[ParamDecl]
    body:   ASTNode     // boxed / arena-allocated
}
```

Using an enum-based AST (vs. class hierarchy) aligns better with V's type
system and makes exhaustive pattern matching in the visitor natural.

**Prerequisite language features:** A1, A2, A3, A7 (ownership / arenas),
A8 (match for visitors).

### Step 4 — `vcc.parser`

The Pratt parser is ~680 lines. In V:

```v
struct Parser {
    ctx:    &mut CompilerContext
    tokens: Vec[Token]
    pos:    i64
}

fn (p: &mut Parser) parse() -> Result[ASTNode, ParseError] { … }
fn (p: &mut Parser) parse_expr(prec: i64) -> Result[ASTNode, ParseError] { … }
```

Error recovery returns `Result` instead of returning `nullptr` on parse
failure.

**Prerequisite language features:** A5 (Result), A8 (match).

### Step 5 — `vcc.semantic`

Two-pass analysis maps naturally to:

```v
fn resolve_names(module: &mut ASTNode, ctx: &mut CompilerContext) -> Result[(), SemanticError]
fn check_types  (module: &mut ASTNode, ctx: &mut CompilerContext) -> Result[(), SemanticError]
```

The `SymbolTable` becomes a `Vec[HashMap[str, Symbol]]` (scoped stack).

**Prerequisite language features:** A1, A2, A5, B2.

### Step 6 — `vcc.ir`

```v
enum Opcode { Add, Sub, Mul, … Alloca, Load, Store, Param, Ret, … }

struct Operand {
    kind:  OperandKind
    reg:   i64
    int_val:   i64
    float_val: f64
    str_val:   str
}

struct IRInstruction {
    opcode:   Opcode
    dest:     i64
    operands: Vec[Operand]
    label:    str
}

struct IRBasicBlock { label: str, instrs: Vec[IRInstruction] }
struct IRFunction   { name:  str, blocks: Vec[IRBasicBlock], reg_counter: i64 }
struct IRModule     { name:  str, functions: Vec[IRFunction] }
```

**Prerequisite language features:** A1, A2, A3, A6 (Vec is generic).

### Step 7 — `vcc.codegen`

Initial strategy: emit textual LLVM IR (Option B from C4).

```v
struct LLVMTextEmitter {
    out: StringBuilder
}

fn (e: &mut LLVMTextEmitter) emit_module(m: &IRModule) -> str {
    // Traverse IRModule and write LLVM IR text
    // Identical logic to C++ LLVMCodeGenerator but writing strings
}
```

Then write the `.ll` file via `std.io.write_file`.

Migrate to LLVM C API (Option A) in a follow-up milestone for performance.

**Prerequisite language features:** A1–A6, B1 (StringBuilder), B3 (file I/O).

### Step 8 — `vcc.driver`

```v
// main.v — entry point of VCC-in-V
import std.os
import vcc.common
import vcc.lexer
import vcc.parser
import vcc.semantic
import vcc.ir
import vcc.codegen

fn main() {
    let args  = os.args()
    let opts  = parse_args(args)
    let compiler = Compiler.new(opts)
    os.exit(compiler.run())
}
```

This is intentionally the last step — it can only be written once all
pipeline modules exist.

---

## Phase E — Self-Hosted Bootstrap

### E1 — Three-Stage Bootstrap Protocol

```
Stage 0  C++ VCC compiles VCC-in-V source
         Produces: vcc-stage1  (native binary built by C++)

Stage 1  vcc-stage1 compiles VCC-in-V source
         Produces: vcc-stage2  (native binary built by stage 1)

Stage 2  vcc-stage2 compiles VCC-in-V source
         Produces: vcc-stage3  (native binary built by stage 2)

Verify:  vcc-stage2 and vcc-stage3 are identical (bit-for-bit or hash match)
         ✓ Bootstrap is complete.
```

The equality check at the end is the gold standard for bootstrap correctness
(Diverse Double-Compiling protocol). If the outputs differ, VCC (V) has a
code-gen bug that only manifests when the compiler runs on itself.

### E2 — Porting Tests from GoogleTest to V

Each C++ test in `tests/test_lexer.cpp`, `test_parser.cpp`, `test_semantic.cpp`,
`test_ir.cpp` must have a V equivalent:

```v
// tests/lexer_test.v
import std.testing
import vcc.lexer

fn test_integer_literal() {
    let toks = Lexer.new("42").tokenize()
    testing.expect_eq(toks[0].kind, TokenKind.IntLiteral)
    testing.expect_eq(toks[0].lexeme, "42")
}
```

The test suite must pass at both stage 1 and stage 2.

### E3 — Archive C++ VCC as Bootstrap Reference

Once stage 2 equals stage 3, commit the last C++ VCC as a tagged release
`v0-bootstrap` and freeze it. It remains the recovery mechanism if VCC (V)
is ever broken and needs to be rebuilt from scratch.

```
git tag v0-bootstrap    # final C++ VCC commit
git branch main         # VCC-in-V is now the production compiler
```

---

## Architecture Changes Required in VCC (C++)

Before Phase D begins, several additions to the C++ compiler are needed to
support the new language features that Phase A introduces:

| Feature | Parser | AST | Semantic | IRGen |
|---|---|---|---|---|
| Struct declaration | `parseStructDecl()` | `StructDecl`, `FieldDecl` | field resolution, struct type | alloca + GEP per field |
| Array/slice literal | `parseArrayLit()` | `ArrayLiteralExpr` | element type check | `alloca [N×T]`, GEP index |
| Enum declaration | `parseEnumDecl()` | `EnumDecl`, `VariantDecl` | discriminant assign | tagged union layout |
| Interface decl | `parseInterfaceDecl()` | `InterfaceDecl` | vtable synthesis | fat-pointer struct + vtable |
| Generic decl | `parseGenericParams()` | type param nodes | monomorphise pass | instantiate per type-arg |
| Option/Result | parser sugar `?` | `OptionType`, `ResultType` | propagation rules | two-field struct |
| Match expression | `parseMatch()` | `MatchExpr` | exhaustiveness check | `switch i64` + branches |
| FFI declaration | `parseExtern()` | `ExternDecl` | ABI annotation | direct LLVM `declare` |
| Owned / arena ptr | ownership syntax | `UniqueType`, `ArenaType` | borrow rules | same as pointer IR |

These changes must be implemented in C++ first so VCC (C++) can compile the
V source code that uses those features (Phase D).

---

## Bootstrap Strategy Summary

```
 ╔══════════════════════════╗
 ║  Phase A–C               ║  Add language features + stdlib to V
 ║  (prerequisite work)     ║  All changes made to VCC (C++)
 ╚══════════╤═══════════════╝
            │
            ▼
 ╔══════════════════════════╗
 ║  Phase D                 ║  Write VCC in V, compiled by VCC (C++)
 ║  VCC-in-V prototype      ║  All 200 tests must pass
 ╚══════════╤═══════════════╝
            │
            ▼
 ╔══════════════════════════╗
 ║  Phase E1                ║  C++ VCC  →  vcc-stage1  (VCC-in-V binary)
 ║  Three-stage bootstrap   ║  stage1   →  vcc-stage2
 ║                          ║  stage2   →  vcc-stage3
 ║                          ║  stage2 == stage3  ✓
 ╚══════════╤═══════════════╝
            │
            ▼
 ╔══════════════════════════╗
 ║  Phase E2–E3             ║  Port all tests to V; tag C++ as v0-bootstrap
 ║  Bootstrap complete      ║  VCC (V) is the production compiler
 ╚══════════════════════════╝
```

---

## Milestone Table

| ID | Milestone | Phase | Prerequisite |
|---|---|---|---|
| M01 | Struct types in V | A | — |
| M02 | Array/slice types | A | M01 |
| M03 | Enum with associated data | A | M01 |
| M04 | Interfaces / vtable dispatch | A | M01 |
| M05 | Option[T] / Result[T,E] | A | M03, M06 |
| M06 | Generics (monomorphisation) | A | M01, M02 |
| M07 | Ownership / arena allocation | A | M01 |
| M08 | Pattern matching (match expr) | A | M03 |
| M09 | std.string (StringBuilder) | B | M01, M02 |
| M10 | std.collections (Vec, HashMap) | B | M06 |
| M11 | std.io (file read/write) | B | M09, M05 |
| M12 | std.os (args, env, exit) | B | M11 |
| M13 | std.fmt (format strings) | B | M09 |
| M14 | std.testing (test runner) | B | M12 |
| M15 | Multi-file module system | C | M10 |
| M16 | Build tool vcc.v | C | M12, M15 |
| M17 | Test runner in V | C | M14, M15 |
| M18 | LLVM C API FFI or text emitter | C | M11 |
| M19 | Port vcc.common → V | D | M01–M03, M09–M11 |
| M20 | Port vcc.lexer → V | D | M19, M10 |
| M21 | Port vcc.ast → V | D | M19, M07, M08 |
| M22 | Port vcc.parser → V | D | M20, M21, M05 |
| M23 | Port vcc.semantic → V | D | M22, M10 |
| M24 | Port vcc.ir → V | D | M19, M10 |
| M25 | Port vcc.codegen → V | D | M24, M18 |
| M26 | Port vcc.driver → V | D | M25, M12 |
| M27 | VCC-in-V passes all 200 tests | D | M26 |
| M28 | Stage-1 bootstrap (C++ → V binary) | E | M27 |
| M29 | Stage-2/3 fixed-point verify | E | M28 |
| M30 | Port test suite to std.testing | E | M17, M27 |
| M31 | Tag v0-bootstrap; retire C++ VCC | E | M29, M30 |

---

## Risk Register

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Memory model choice blocks progress | High | High | Commit to arena allocation early (M07); do not attempt ownership inference in Phase A |
| Generics monomorphisation causes code-size explosion | Medium | Medium | Add a threshold-based shared-template fallback; fine-tune after bootstrap |
| LLVM C API FFI complexity delays codegen | Medium | High | Use textual `.ll` emission (Option B) for the prototype; migrate to C API post-bootstrap |
| Stage-2 ≠ Stage-3 (codegen non-determinism) | Low | High | Sort all output (symbol names, block labels) deterministically from the start |
| V stdlib too slow for production use | Low | Medium | Profile after bootstrap; standard library can be optimised in V once self-hosted |
| C++ VCC bugs exposed by self-hosted test suite | Medium | Low | Each porting step increases test coverage; bugs found in Phase D are fixed in C++ first |

---

## Quick-Start: First Three Months

If you start today with the C++ VCC as-is, the highest-value first steps are:

1. **Decide the memory model** (arena vs. unique vs. GC). Document it.
   This unblocks M07 and therefore M21 (AST porting).

2. **Add struct syntax to the C++ parser and IRGen** (M01).
   Structs unlock all subsequent language work. Two weeks of focused C++ work.

3. **Add `Vec[T]` and `HashMap[K,V]` with monomorphisation** (M02, M06, M10).
   These are the most-used stdlib types. Required for M20 (lexer port).

4. **Write the lexer in V** (M20) — it has no dependencies except `Token`
   (structs + enums) and `HashMap` (keyword map). It is the smallest,
   most self-contained module.  A working V lexer is the first proof that
   the language can write compiler code.

5. **Write a test in V** that feeds the V lexer output into the C++ parser.
   This is the first hybrid pipeline and a forcing function to fix issues in
   the stdlib and type system before committing to a full port.
