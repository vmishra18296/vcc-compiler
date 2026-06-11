#pragma once

#include "vcc/ir/IRModule.h"

#include <filesystem>
#include <ostream>
#include <string>

/// @file CodeGen.h
/// Abstract code-generation interface for VCC.
///
/// Design
/// ──────
///  CodeGen is the abstract base class all backends must implement.
///  Currently two backends are planned:
///
///   • IRTextEmitter (Phase 1) – text-serialises VCC IR to a file.
///   • LLVMBackend   (Phase 3) – lowers VCC IR to native code via LLVM.
///
///  Callers use CodeGen through this interface so the driver is decoupled
///  from any concrete backend.
///
/// Future extensions
/// ─────────────────
///  • Wasm backend.
///  • QBE/C intermediate backends for bootstrapping.
///  • Optimisation pipeline integration.

namespace vcc::codegen {

class CodeGen {
public:
    virtual ~CodeGen() = default;

    /// Generate output for the given IR module.
    /// @param module   Fully lowered IR ready for code generation.
    /// @param outPath  Output file path.  The backend decides the format.
    /// @returns true iff generation succeeded.
    [[nodiscard]] virtual bool generate(const ir::IRModule& module,
                                        const std::filesystem::path& outPath) = 0;

    /// Human-readable name of this backend (for diagnostics and --version).
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

// ─── IRTextEmitter ────────────────────────────────────────────────────────────

/// A simple backend that serialises VCC IR to human-readable text.
/// Useful for --dump-ir and debugging without LLVM installed.
class IRTextEmitter : public CodeGen {
public:
    [[nodiscard]] bool generate(const ir::IRModule& module,
                                const std::filesystem::path& outPath) override;

    [[nodiscard]] std::string_view name() const noexcept override { return "ir-text"; }

    /// Write IR text to any output stream (used by --dump-ir).
    void emit(const ir::IRModule& module, std::ostream& out) const;
};

} // namespace vcc::codegen
