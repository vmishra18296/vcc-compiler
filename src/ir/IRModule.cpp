#include "vcc/ir/IRModule.h"
#include "vcc/ir/IRPrinter.h"

#include <iostream>

namespace vcc::ir {

// ─── IRBasicBlock ─────────────────────────────────────────────────────────────

bool IRBasicBlock::isTerminated() const noexcept {
    return !instrs_.empty() && instrs_.back().isTerminator();
}

// ─── IRFunction ───────────────────────────────────────────────────────────────

IRBasicBlock& IRFunction::addBlock(std::string label) {
    blocks_.push_back(std::make_unique<IRBasicBlock>(std::move(label)));
    return *blocks_.back();
}

IRBasicBlock* IRFunction::entryBlock() noexcept {
    return blocks_.empty() ? nullptr : blocks_.front().get();
}

void IRFunction::dump() const {
    IRPrinter printer(std::cout);
    printer.print(*this);
}

void IRFunction::print(std::ostream& out) const {
    IRPrinter printer(out);
    printer.print(*this);
}

// ─── IRModule ────────────────────────────────────────────────────────────────

IRFunction& IRModule::addFunction(std::string name) {
    functions_.push_back(std::make_unique<IRFunction>(std::move(name)));
    return *functions_.back();
}

void IRModule::dump() const {
    IRPrinter printer(std::cout);
    printer.print(*this);
}

void IRModule::print(std::ostream& out) const {
    IRPrinter printer(out);
    printer.print(*this);
}

} // namespace vcc::ir
