# VCC — V Compiler Collection

VCC is a production-quality, self-hosted compiler for the **V programming language**, written in modern C++20.

---

## Project Status

> Phase 1 – Frontend (Lexer → Parser → AST → Semantic Analysis)  
> Phase 2 – IR generation (VCC IR)  
> Phase 3 – LLVM backend & code generation  

---

## Architecture

```
Source text
    │
    ▼
┌─────────┐    tokens   ┌────────┐    AST    ┌──────────┐    typed-AST
│  Lexer  │ ──────────► │ Parser │ ────────► │ Semantic │ ─────────────►
└─────────┘             └────────┘           │ Analysis │
                                             └──────────┘
                                                   │
                              ┌────────────────────┘
                              ▼
                        ┌──────────┐    IR     ┌─────────┐    object
                        │  IRGen   │ ────────► │ CodeGen │ ──────────►
                        └──────────┘           └─────────┘
```

### Module map

| Directory            | Responsibility                                      |
|----------------------|-----------------------------------------------------|
| `src/common`         | Source locations, diagnostics, tokens               |
| `src/lexer`          | Tokeniser / scanner                                 |
| `src/ast`            | AST node hierarchy (Decl / Stmt / Expr / Type)      |
| `src/parser`         | Recursive-descent / Pratt parser                    |
| `src/semantic`       | Name resolution, type checking, symbol table        |
| `src/ir`             | VCC intermediate representation                    |
| `src/codegen`        | Target-specific code generation (LLVM Phase 2)      |
| `src/driver`         | CLI driver (`vcc` binary)                           |
| `tests/`             | GoogleTest unit tests per module                    |
| `tools/`             | Developer tools (AST dumper, IR printer, …)         |
| `examples/`          | Sample V programs                                   |
| `docs/`              | Language specification, architecture notes          |

---

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

Build with LLVM backend (requires LLVM 17+):

```bash
cmake -S . -B build -DVCC_ENABLE_LLVM=ON
cmake --build build -j$(nproc)
```

---

## Usage

```bash
# Compile a V source file
./build/src/driver/vcc hello.v -o hello

# Dump tokens
./build/src/driver/vcc --dump-tokens hello.v

# Dump AST
./build/src/driver/vcc --dump-ast hello.v

# Dump IR
./build/src/driver/vcc --dump-ir hello.v
```

---

## V Language – Quick Taste

```v
module main

import std.io

fn fibonacci(n: i64) -> i64 {
    if n <= 1 {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

fn main() {
    let result = fibonacci(10)
    io.println("fib(10) = " + result.to_string())
}
```

---

## Contributing

1. Follow the C++20 coding style already established in the codebase.
2. Every new feature must be covered by tests in `tests/`.
3. Run `clang-format` and `clang-tidy` before committing.

---

## License

MIT – see `LICENSE`.
# vcc-compiler
