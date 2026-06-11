# VCC Architecture Notes

## Overview

VCC is structured as a set of independent static libraries linked by the `vcc`
binary (the driver).  Each library is a separate CMake target, which means:

* Individual phases can be unit-tested in isolation.
* Future IDE / LSP integrations can reuse the lexer or parser without pulling
  in the backend.
* The LLVM backend is an optional dependency behind `VCC_ENABLE_LLVM`.

---

## Pipeline

```
Source text
    │
    ▼ vcc::lexer::Lexer
Token stream  (std::vector<Token>)
    │
    ▼ vcc::parser::Parser
AST root  (std::unique_ptr<ModuleDecl>)
    │
    ▼ vcc::semantic::SemanticAnalyzer
  Pass 1 – name resolution  (SymbolTable)
  Pass 2 – type checking    (TypeChecker)
    │
    ▼ vcc::ir::IRGen
IRModule  (VCC 3-address IR)
    │
    ▼ vcc::codegen::CodeGen  (abstract)
  IRTextEmitter   – textual VCC IR  (Phase 1)
  LLVMBackend     – native code     (Phase 3)
```

---

## Module responsibilities

### `vcc_common`

| File                | Role                                              |
|---------------------|---------------------------------------------------|
| `SourceLocation.h`  | Value types: `SourceLocation`, `SourceRange`      |
| `Diagnostics.h/cpp` | Emit, collect, and print compiler diagnostics     |
| `Token.h/cpp`       | `TokenKind` enum, `Token` struct, keyword map     |
| `CompilerContext.h` | Shared state: options, diagnostics, source files  |

### `vcc_lexer`

Single-pass, character-level scanner.  Produces a flat vector of `Token`s.
Handles all V literals, operators, and identifiers.  Keyword classification
is done via a hash-map lookup.

### `vcc_ast`

Node hierarchy rooted at `ASTNode`:

```
ASTNode
├── Decl      (Declarations.h)
├── Stmt      (Statements.h)
├── Expr      (Expressions.h)
└── TypeNode  (Types.h)
```

All nodes are heap-allocated and owned exclusively via `std::unique_ptr`.
The `ASTVisitor` / `ASTVisitorBase` pair enables the visitor pattern.

### `vcc_parser`

Recursive-descent parser with a Pratt (top-down operator-precedence) sub-parser
for expressions.  Error recovery synchronises at statement boundaries.

### `vcc_semantic`

Two passes:
1. **Name resolution** (`SemanticAnalyzer`) – populates `SymbolTable`, detects
   undeclared identifiers and redefinitions.
2. **Type checking** (`TypeChecker`) – validates types of expressions and
   statements.  Currently string-based; Phase 3 introduces a `Type*` hierarchy.

### `vcc_ir`

3-address, SSA-inspired intermediate representation:

```
IRModule
└── IRFunction
    └── IRBasicBlock
        └── IRInstruction  { opcode, dest, operands }
```

`IRGen` lowers the typed AST to this IR in a single pass.

### `vcc_codegen`

Abstract `CodeGen` interface with two implementations:

| Backend        | Status       | Description                    |
|----------------|-------------|--------------------------------|
| `IRTextEmitter`| Implemented  | Human-readable VCC IR text     |
| `LLVMBackend`  | Stub (Phase 3)| LLVM-based native code gen    |

### `vcc_driver_lib` / `vcc`

`Compiler` orchestrates all phases.  `main.cpp` provides CLI argument parsing.

---

## Dependency graph (simplified)

```
vcc  ──►  vcc_driver_lib
           ├─► vcc_codegen  ──► vcc_ir  ──► vcc_ast  ──► vcc_common
           ├─► vcc_semantic ──► vcc_ast
           ├─► vcc_parser   ──► vcc_ast
           └─► vcc_lexer    ──► vcc_common
```

---

## Adding a new compiler pass

1. Create `include/vcc/<module>/MyPass.h` – inherit from `ASTVisitorBase`.
2. Create `src/<module>/MyPass.cpp` – implement the visit methods.
3. Add the `.cpp` to `src/<module>/CMakeLists.txt`.
4. Call the pass from `Compiler.cpp` in the appropriate phase slot.
5. Add tests in `tests/test_<module>.cpp`.

---

## Testing strategy

* **Unit tests** (GoogleTest) live in `tests/` and test one module at a time.
* **Integration tests** (future) will compile example `.v` files end-to-end.
* Run with `ctest --test-dir build`.

---

## Coding conventions

* C++20 throughout.
* `[[nodiscard]]` on all functions returning values the caller must not ignore.
* `std::unique_ptr` for single-owner heap objects; raw pointers only for
  non-owning references.
* No raw `new`/`delete` outside `std::make_unique`.
* All public API in header files under `include/vcc/`.
* Internal helpers stay in the `.cpp` file or an anonymous namespace.
