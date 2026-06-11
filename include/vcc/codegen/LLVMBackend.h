#pragma once

#include "CodeGen.h"
#include "vcc/common/CompilerContext.h"

/// @file LLVMBackend.h
/// LLVM-based native code generation backend (Phase 3 stub).
///
/// When VCC_ENABLE_LLVM=ON this backend:
///  1. Walks the VCC IRModule.
///  2. Emits an llvm::Module using the LLVM C++ API.
///  3. Runs the LLVM optimisation pipeline (O0–O3).
///  4. Emits object code or LLVM bitcode depending on the output format.
///
/// Current state
/// ─────────────
///  This is a Phase-3 stub.  The class compiles but generate() returns false
///  with a "LLVM backend not yet implemented" note.  This allows the rest of
///  the compiler to be tested end-to-end without LLVM.
///
/// Enabling
/// ────────
///  cmake -DVCC_ENABLE_LLVM=ON …
///
/// Future extensions
/// ─────────────────
///  • Full VCC IR → LLVM IR lowering.
///  • Per-function optimisation passes.
///  • Debug info (DWARF) emission.
///  • Cross-compilation via LLVM target triples.

namespace vcc::codegen {

class LLVMBackend : public CodeGen {
public:
    explicit LLVMBackend(common::CompilerContext& ctx);

    [[nodiscard]] bool generate(const ir::IRModule& module,
                                const std::filesystem::path& outPath) override;

    [[nodiscard]] std::string_view name() const noexcept override { return "llvm"; }

private:
    common::CompilerContext& ctx_;
};

} // namespace vcc::codegen
