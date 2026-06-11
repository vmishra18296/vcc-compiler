#pragma once

#include "IRInstruction.h"

#include <memory>
#include <ostream>
#include <string>
#include <vector>

/// @file IRModule.h
/// VCC IR module, function, and basic-block containers.
///
/// Hierarchy
/// ─────────
///   IRModule
///   └── IRFunction  (one per source-level fn)
///       └── IRBasicBlock  (one per control-flow branch target)
///           └── IRInstruction  (one per 3-address operation)
///
/// Future extensions
/// ─────────────────
///  • IRGlobal for module-level constants and static variables.
///  • IRType hierarchy for the type system.
///  • Debug info attachment (DILocation → DW_AT_… equivalents).

namespace vcc::ir {

// ─── IRBasicBlock ─────────────────────────────────────────────────────────────

class IRBasicBlock {
public:
    explicit IRBasicBlock(std::string label) : label_(std::move(label)) {}

    [[nodiscard]] const std::string&              label()        const noexcept { return label_; }
    [[nodiscard]] const std::vector<IRInstruction>& instructions() const noexcept { return instrs_; }
    [[nodiscard]] std::vector<IRInstruction>&       instructions()       noexcept { return instrs_; }

    void append(IRInstruction instr) { instrs_.push_back(std::move(instr)); }

    [[nodiscard]] bool isTerminated() const noexcept;

private:
    std::string               label_;
    std::vector<IRInstruction> instrs_;
};

// ─── IRFunction ───────────────────────────────────────────────────────────────

class IRFunction {
public:
    explicit IRFunction(std::string name) : name_(std::move(name)) {}

    [[nodiscard]] const std::string&                       name()   const noexcept { return name_; }
    [[nodiscard]] const std::vector<std::unique_ptr<IRBasicBlock>>& blocks() const noexcept { return blocks_; }

    /// Create and return a new basic block appended to this function.
    IRBasicBlock& addBlock(std::string label);

    /// The entry basic block (first block).
    [[nodiscard]] IRBasicBlock* entryBlock() noexcept;

    /// Allocate the next virtual register ID.
    [[nodiscard]] RegID nextReg() noexcept { return ++regCounter_; }

    /// Dump a textual representation to stdout (debug utility).
    void dump() const;

    /// Print a textual representation to @p out using the canonical IRPrinter
    /// format (named variables + t1/t2/… temporaries).
    void print(std::ostream& out) const;

private:
    std::string                                  name_;
    std::vector<std::unique_ptr<IRBasicBlock>>   blocks_;
    RegID                                        regCounter_{0};
};

// ─── IRModule ────────────────────────────────────────────────────────────────

class IRModule {
public:
    explicit IRModule(std::string name) : name_(std::move(name)) {}

    [[nodiscard]] const std::string&                      name()      const noexcept { return name_; }
    [[nodiscard]] const std::vector<std::unique_ptr<IRFunction>>& functions() const noexcept { return functions_; }

    /// Create and return a new function in this module.
    IRFunction& addFunction(std::string name);

    /// Dump the full module (debug utility).
    void dump() const;

    /// Print the full module to @p out using the canonical IRPrinter format.
    void print(std::ostream& out) const;

private:
    std::string                                name_;
    std::vector<std::unique_ptr<IRFunction>>   functions_;
};

} // namespace vcc::ir
