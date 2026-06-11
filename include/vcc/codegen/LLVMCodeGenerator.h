#pragma once

#include "vcc/ir/IRModule.h"

#include <filesystem>
#include <memory>

// Forward-declare LLVM heavyweights so consumers of this header
// do not need to pull in any LLVM headers.
namespace llvm {
class LLVMContext;
class Module;
} // namespace llvm

/// @file LLVMCodeGenerator.h
/// Lowers a VCC IRModule to LLVM IR and writes a `.ll` text file.
///
/// Supported opcodes (Phase 1)
/// ────────────────────────────
///  Memory   : Alloca, Load, Store, Param
///  Literals : Copy (integer, float, bool)
///  Arithmetic: Add, Sub, Mul, Div, Mod, Neg
///  Bitwise  : BitAnd, BitOr, BitXor, BitNot, Shl, Shr
///  Compare  : CmpEq, CmpNe, CmpLt, CmpGt, CmpLe, CmpGe  (→ i64 via zext)
///  Logical  : LogAnd, LogOr, LogNot
///  Control  : Ret, Branch, CondBranch
///  Calls    : Call (direct, by name)
///  Cast     : identity pass-through in Phase 1
///
/// All VCC IR values are treated as `i64`.  Boolean results (compare, logical)
/// are widened back to `i64` via `zext`.  Branch conditions are narrowed to
/// `i1` via `icmp ne <val>, 0`.
///
/// Usage
/// ─────
/// @code
///   vcc::codegen::LLVMCodeGenerator gen;
///   gen.emitIR(*irModule, "output.ll");
/// @endcode

namespace vcc::codegen {

class LLVMCodeGenerator {
public:
    LLVMCodeGenerator();

    /// Destructor defined in the .cpp so unique_ptr<LLVMContext> is valid
    /// despite LLVMContext being an incomplete type in this header.
    ~LLVMCodeGenerator();

    // Non-copyable; move-only.
    LLVMCodeGenerator(const LLVMCodeGenerator&)            = delete;
    LLVMCodeGenerator& operator=(const LLVMCodeGenerator&) = delete;
    LLVMCodeGenerator(LLVMCodeGenerator&&)                 = default;
    LLVMCodeGenerator& operator=(LLVMCodeGenerator&&)      = default;

    // ── Core API ──────────────────────────────────────────────────────────────

    /// Lower @p irModule to an `llvm::Module`.
    /// Returns nullptr on failure (verification error or empty module).
    [[nodiscard]] std::unique_ptr<llvm::Module>
    lower(const ir::IRModule& irModule);

    /// Lower @p irModule and write LLVM IR text (`*.ll`) to @p outputPath.
    /// Returns false if lowering or file I/O fails.
    bool emitIR(const ir::IRModule& irModule,
                const std::filesystem::path& outputPath);

private:
    std::unique_ptr<llvm::LLVMContext> ctx_;
};

} // namespace vcc::codegen
