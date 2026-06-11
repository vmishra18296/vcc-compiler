#pragma once

#include "IRInstruction.h"
#include "IRModule.h"

#include <ostream>
#include <string>
#include <unordered_map>

/// @file IRPrinter.h
/// Human-readable text printer for the VCC Intermediate Representation.
///
/// Output format
/// ─────────────
/// Registers produced by alloca instructions are printed using the declared
/// variable name.  All other result registers get sequential "t1", "t2", …
/// names so the output mirrors familiar 3-address-code notation:
///
/// @code
///   ; VCC IR – module "example"
///
///   fn add:
///   entry:
///       x = alloca
///       y = alloca
///       t1 = load x
///       t2 = load y
///       t3 = add t1 t2
///       ret t3
/// @endcode
///
/// Usage
/// ─────
/// @code
///   IRPrinter printer(std::cout);
///   printer.print(*irModule);
/// @endcode
///
/// The module's dump() and each function's dump() delegate here so that the
/// --dump-ir flag and debug helpers share a single canonical format.

namespace vcc::ir {

class IRPrinter {
public:
    explicit IRPrinter(std::ostream& out);

    // ── Top-level entry points ─────────────────────────────────────────────

    /// Print the full module (banner + every function).
    void print(const IRModule& module);

    /// Print a single function.
    void print(const IRFunction& fn);

private:
    // ── Name map ──────────────────────────────────────────────────────────────
    /// Maps RegID → human-readable name ("x", "y", "t1", "t2", …).
    using NameMap = std::unordered_map<RegID, std::string>;

    /// Scan every instruction in @p fn and build the NameMap:
    ///  • alloca with a label  →  use the label as the variable name
    ///  • every other dest     →  assign "t{N}" in order of first appearance
    [[nodiscard]] static NameMap buildNameMap(const IRFunction& fn);

    // ── Block / instruction printers ──────────────────────────────────────────

    void printBlock(const IRBasicBlock& bb, const NameMap& names);
    void printInstr(const IRInstruction& instr, const NameMap& names);

    // ── Formatting helpers ────────────────────────────────────────────────────

    /// Return the printable name for register @p r.
    [[nodiscard]] std::string regStr(RegID r, const NameMap& names) const;

    /// Return the printable string for a single operand.
    [[nodiscard]] std::string operandStr(const Operand& op,
                                         const NameMap& names) const;

    std::ostream& out_;
};

} // namespace vcc::ir
