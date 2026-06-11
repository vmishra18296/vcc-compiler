#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

/// @file IRInstruction.h
/// VCC Intermediate Representation – instruction definitions.
///
/// Design
/// ──────
///  VCC IR is a simple, linear, SSA-inspired 3-address code IR.
///  It sits between the typed AST and the LLVM IR (Phase 3).
///
///  Each instruction produces at most one virtual register result.
///  Basic blocks contain a flat list of instructions terminated by
///  a terminator (Ret, Branch, CondBranch).
///
/// Future extensions
/// ─────────────────
///  • Full SSA φ-nodes once the dominance analysis is implemented.
///  • Intrinsics for memory operations (load, store, alloca).
///  • Type-annotated values for the LLVM lowering pass.

namespace vcc::ir {

// ─── Value references ─────────────────────────────────────────────────────────

/// A virtual register identifier.  Represents an SSA value.
using RegID = uint32_t;
constexpr RegID NoReg = 0;  ///< Sentinel: instruction produces no value

/// Operand to an instruction – either a virtual register or an immediate.
struct Operand {
    enum class Kind { Reg, IntImm, FloatImm, StringRef, BoolImm } kind;
    RegID       reg{NoReg};
    int64_t     intVal{0};
    double      floatVal{0.0};
    std::string strVal;

    static Operand fromReg(RegID r)         { return {Kind::Reg,       r, 0, 0.0, {}}; }
    static Operand fromInt(int64_t v)       { return {Kind::IntImm,    0, v, 0.0, {}}; }
    static Operand fromFloat(double v)      { return {Kind::FloatImm,  0, 0, v,   {}}; }
    static Operand fromString(std::string s){ return {Kind::StringRef, 0, 0, 0.0, std::move(s)}; }
    static Operand fromBool(bool v)         { return {Kind::BoolImm,   0, v ? 1 : 0, 0.0, {}}; }
};

// ─── Instruction opcodes ──────────────────────────────────────────────────────

enum class Opcode : uint16_t {
    // ── Arithmetic ───────────────────────────────────────────────────────────
    Add, Sub, Mul, Div, Mod,
    Neg,  ///< Unary negation

    // ── Bitwise ──────────────────────────────────────────────────────────────
    BitAnd, BitOr, BitXor, BitNot,
    Shl, Shr,

    // ── Comparison ───────────────────────────────────────────────────────────
    CmpEq, CmpNe, CmpLt, CmpGt, CmpLe, CmpGe,

    // ── Logical ──────────────────────────────────────────────────────────────
    LogAnd, LogOr, LogNot,

    // ── Memory ───────────────────────────────────────────────────────────────
    Alloca,  ///< Stack allocation: result = alloca <type>
    Load,    ///< Load from address: result = load ptr
    Store,   ///< Store to address: store val, ptr  (no result)

    // ── Control flow (terminators) ───────────────────────────────────────────
    Ret,         ///< Return from function  (operand = return value or empty)
    Branch,      ///< Unconditional branch to a basic block
    CondBranch,  ///< Conditional branch: cond, true-bb, false-bb

    // ── Calls ────────────────────────────────────────────────────────────────
    Call,   ///< result = call fn(args…)
    // ── Parameters ────────────────────────────────────────────────────
    Param,  ///< result = param N  — retrieve the Nth incoming function argument
    // ── Miscellaneous ────────────────────────────────────────────────────────
    Copy,   ///< result = operand  (move / register copy)
    Cast,   ///< result = cast operand to <type>
    Phi,    ///< SSA φ-node (Phase 3)
};

[[nodiscard]] std::string_view opcodeName(Opcode op) noexcept;

// ─── IRInstruction ────────────────────────────────────────────────────────────

struct IRInstruction {
    Opcode              opcode;
    RegID               dest{NoReg};       ///< Result register (0 = no result)
    std::vector<Operand> operands;          ///< Input operands
    std::string         label;             ///< Optional human-readable label / name
    std::string         type;              ///< Type annotation (future: Type*)

    /// True iff this instruction is a basic-block terminator.
    [[nodiscard]] bool isTerminator() const noexcept;
};

} // namespace vcc::ir
