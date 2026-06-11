#pragma once

#include "IRInstruction.h"
#include "IRModule.h"

#include <string>
#include <string_view>
#include <vector>

/// @file IRBuilder.h
/// Fluent helper for emitting VCC IR instructions.
///
/// IRBuilder owns the current "insert point" (function + basic block).
/// Every build*() method:
///   1. Allocates a fresh virtual register via the parent IRFunction.
///   2. Constructs the corresponding IRInstruction.
///   3. Appends it to the current IRBasicBlock.
///   4. Returns the destination register (or NoReg for terminators/stores).
///
/// Usage:
/// @code
///   IRBuilder b;
///   b.reset(fn, entryBlock);
///
///   RegID x   = b.buildAlloca("x");     // x = alloca
///   RegID t0  = b.buildParam(0);        // t0 = param 0
///   b.buildStore(t0, x);                // store t0 x
///   RegID t1  = b.buildLoad(x);         // t1 = load x
///   RegID t2  = b.buildInt(1);          // t2 = copy 1
///   RegID t3  = b.buildBinOp(Opcode::Add, t1, t2); // t3 = add t1 t2
///   b.buildRet(t3);                     // ret t3
/// @endcode

namespace vcc::ir {

class IRBuilder {
public:
    IRBuilder() = default;

    // ── Attach / detach ───────────────────────────────────────────────────────

    /// Attach the builder to @p fn, starting insertion at @p entry.
    void reset(IRFunction& fn, IRBasicBlock& entry) noexcept;

    /// Detach the builder (call at end of function code-gen).
    void reset() noexcept;

    // ── Insert-point management ───────────────────────────────────────────────

    void          setBlock(IRBasicBlock& bb) noexcept;
    IRBasicBlock& block() const noexcept;
    bool          hasBlock() const noexcept;

    /// True iff the current block already ends with a terminator.
    [[nodiscard]] bool isTerminated() const noexcept;

    /// Create and append a new basic block to the current function.
    IRBasicBlock& addBlock(std::string label);

    /// Allocate a unique label string: "<prefix>_N".
    [[nodiscard]] std::string newLabel(std::string_view prefix);

    // ── Register allocation ───────────────────────────────────────────────────

    [[nodiscard]] RegID newReg();

    // ── Memory instructions ───────────────────────────────────────────────────

    /// `varName = alloca`   — named stack slot for a local variable/parameter.
    [[nodiscard]] RegID buildAlloca(std::string varName);

    /// `tN = load ptrReg`   — load from a register that holds an address.
    [[nodiscard]] RegID buildLoad(RegID ptrReg);

    /// `tN = load symbolName`  — symbolic load from a global / undeclared name.
    [[nodiscard]] RegID buildLoadNamed(std::string symbolName);

    /// `store valueReg ptrReg`  — store a value to an address.
    void buildStore(RegID valueReg, RegID ptrReg);

    // ── Literal / immediate values ────────────────────────────────────────────

    [[nodiscard]] RegID buildInt   (int64_t    val);
    [[nodiscard]] RegID buildFloat (double     val);
    [[nodiscard]] RegID buildBool  (bool       val);
    [[nodiscard]] RegID buildString(std::string val);
    [[nodiscard]] RegID buildChar  (char32_t   val);
    [[nodiscard]] RegID buildNil   ();

    // ── Parameter retrieval ───────────────────────────────────────────────────

    /// `tN = param index`  — retrieve the @p index-th incoming argument.
    [[nodiscard]] RegID buildParam(uint32_t index);

    // ── Arithmetic / logic / comparison ──────────────────────────────────────

    [[nodiscard]] RegID buildBinOp(Opcode op, RegID lhs, RegID rhs);
    [[nodiscard]] RegID buildUnOp (Opcode op, RegID operand);
    [[nodiscard]] RegID buildCast (RegID value, std::string toType);

    // ── Calls ─────────────────────────────────────────────────────────────────

    [[nodiscard]] RegID buildCall(std::string callee, const std::vector<RegID>& args);

    // ── Terminators ──────────────────────────────────────────────────────────

    void buildRet       (RegID valueReg = NoReg);
    void buildBranch    (std::string_view targetLabel);
    void buildCondBranch(RegID condReg,
                         std::string_view trueLabel,
                         std::string_view falseLabel);

private:
    void append(IRInstruction instr);

    IRFunction*   fn_{nullptr};
    IRBasicBlock* bb_{nullptr};
    uint32_t      labelCounter_{0};
};

} // namespace vcc::ir
