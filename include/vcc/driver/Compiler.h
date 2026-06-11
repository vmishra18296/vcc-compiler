#pragma once

#include "vcc/common/CompilerContext.h"

/// @file Compiler.h
/// High-level compiler driver that orchestrates all phases.
///
/// Phases
/// ──────
///  1. Read source file → register with CompilerContext
///  2. Lex     (Lexer → token stream)
///  3. Parse   (Parser → AST)
///  4. Analyse (SemanticAnalyzer → name resolution + type checking)
///  5. IRGen   (IRGen → VCC IR)
///  6. CodeGen (backend → output file)
///
/// Each phase aborts early if diagnostics contain errors, preventing
/// cascading false positives.

namespace vcc::driver {

class Compiler {
public:
    explicit Compiler(common::CompilerOptions opts = {});

    /// Run all compilation phases for the configured input files.
    /// @returns 0 on success, non-zero on failure.
    [[nodiscard]] int run();

private:
    common::CompilerContext ctx_;

    /// Compile a single source file.  Returns false on any error.
    [[nodiscard]] bool compileFile(const std::filesystem::path& path);
};

} // namespace vcc::driver
