#include "vcc/ir/IRBuilder.h"

#include <cassert>
#include <string>

namespace vcc::ir {

// ─── Attach / detach ──────────────────────────────────────────────────────────

void IRBuilder::reset(IRFunction& fn, IRBasicBlock& entry) noexcept {
    fn_           = &fn;
    bb_           = &entry;
    labelCounter_ = 0;
}

void IRBuilder::reset() noexcept {
    fn_           = nullptr;
    bb_           = nullptr;
    labelCounter_ = 0;
}

// ─── Insert-point management ──────────────────────────────────────────────────

void IRBuilder::setBlock(IRBasicBlock& bb) noexcept {
    bb_ = &bb;
}

IRBasicBlock& IRBuilder::block() const noexcept {
    assert(bb_ && "IRBuilder: no active basic block");
    return *bb_;
}

bool IRBuilder::hasBlock() const noexcept { return bb_ != nullptr; }

bool IRBuilder::isTerminated() const noexcept {
    return bb_ && bb_->isTerminated();
}

IRBasicBlock& IRBuilder::addBlock(std::string label) {
    assert(fn_ && "IRBuilder: no active function");
    return fn_->addBlock(std::move(label));
}

std::string IRBuilder::newLabel(std::string_view prefix) {
    return std::string(prefix) + "_" + std::to_string(labelCounter_++);
}

RegID IRBuilder::newReg() {
    assert(fn_ && "IRBuilder: no active function");
    return fn_->nextReg();
}

// ─── Append helper ────────────────────────────────────────────────────────────

void IRBuilder::append(IRInstruction instr) {
    assert(bb_ && "IRBuilder: no active basic block");
    bb_->append(std::move(instr));
}

// ─── Memory instructions ──────────────────────────────────────────────────────

RegID IRBuilder::buildAlloca(std::string varName) {
    RegID r = newReg();
    append({Opcode::Alloca, r, {}, std::move(varName)});
    return r;
}

RegID IRBuilder::buildLoad(RegID ptrReg) {
    RegID r = newReg();
    append({Opcode::Load, r, {Operand::fromReg(ptrReg)}});
    return r;
}

RegID IRBuilder::buildLoadNamed(std::string symbolName) {
    RegID r = newReg();
    append({Opcode::Load, r, {}, std::move(symbolName)});
    return r;
}

void IRBuilder::buildStore(RegID valueReg, RegID ptrReg) {
    append({Opcode::Store, NoReg,
            {Operand::fromReg(valueReg), Operand::fromReg(ptrReg)}});
}

// ─── Literal / immediate values ───────────────────────────────────────────────

RegID IRBuilder::buildInt(int64_t val) {
    RegID r = newReg();
    append({Opcode::Copy, r, {Operand::fromInt(val)}});
    return r;
}

RegID IRBuilder::buildFloat(double val) {
    RegID r = newReg();
    append({Opcode::Copy, r, {Operand::fromFloat(val)}});
    return r;
}

RegID IRBuilder::buildBool(bool val) {
    RegID r = newReg();
    append({Opcode::Copy, r, {Operand::fromBool(val)}});
    return r;
}

RegID IRBuilder::buildString(std::string val) {
    RegID r = newReg();
    append({Opcode::Copy, r, {Operand::fromString(std::move(val))}});
    return r;
}

RegID IRBuilder::buildChar(char32_t val) {
    RegID r = newReg();
    append({Opcode::Copy, r, {Operand::fromInt(static_cast<int64_t>(val))}});
    return r;
}

RegID IRBuilder::buildNil() {
    RegID r = newReg();
    append({Opcode::Copy, r, {Operand::fromInt(0)}, "nil"});
    return r;
}

// ─── Parameter retrieval ──────────────────────────────────────────────────────

RegID IRBuilder::buildParam(uint32_t index) {
    RegID r = newReg();
    append({Opcode::Param, r, {Operand::fromInt(static_cast<int64_t>(index))}});
    return r;
}

// ─── Arithmetic / logic / comparison ─────────────────────────────────────────

RegID IRBuilder::buildBinOp(Opcode op, RegID lhs, RegID rhs) {
    RegID r = newReg();
    append({op, r, {Operand::fromReg(lhs), Operand::fromReg(rhs)}});
    return r;
}

RegID IRBuilder::buildUnOp(Opcode op, RegID operand) {
    RegID r = newReg();
    append({op, r, {Operand::fromReg(operand)}});
    return r;
}

RegID IRBuilder::buildCast(RegID value, std::string toType) {
    RegID r = newReg();
    append({Opcode::Cast, r, {Operand::fromReg(value)}, std::move(toType)});
    return r;
}

// ─── Calls ────────────────────────────────────────────────────────────────────

RegID IRBuilder::buildCall(std::string callee, const std::vector<RegID>& args) {
    RegID r = newReg();
    std::vector<Operand> ops;
    ops.reserve(args.size());
    for (RegID a : args) ops.push_back(Operand::fromReg(a));
    append({Opcode::Call, r, std::move(ops), std::move(callee)});
    return r;
}

// ─── Terminators ─────────────────────────────────────────────────────────────

void IRBuilder::buildRet(RegID valueReg) {
    if (valueReg == NoReg)
        append({Opcode::Ret, NoReg, {}});
    else
        append({Opcode::Ret, NoReg, {Operand::fromReg(valueReg)}});
}

void IRBuilder::buildBranch(std::string_view targetLabel) {
    append({Opcode::Branch, NoReg, {}, std::string(targetLabel)});
}

void IRBuilder::buildCondBranch(RegID            condReg,
                                 std::string_view trueLabel,
                                 std::string_view falseLabel) {
    append({Opcode::CondBranch, NoReg,
            {Operand::fromReg(condReg)},
            std::string(trueLabel) + "," + std::string(falseLabel)});
}

} // namespace vcc::ir
