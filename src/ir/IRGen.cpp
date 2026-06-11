#include "vcc/ir/IRGen.h"

#include "vcc/ast/Statements.h"

#include <cassert>

namespace vcc::ir {

using namespace vcc::ast;
using namespace vcc::common;

IRGen::IRGen(CompilerContext& ctx) : ctx_(ctx) {}

std::unique_ptr<IRModule> IRGen::generate(ModuleDecl& module) {
    module_ = std::make_unique<IRModule>(module.name());
    module.accept(*this);
    return std::move(module_);
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

Opcode IRGen::binaryOpToOpcode(BinaryOp op) const noexcept {
    switch (op) {
        case BinaryOp::Add:    return Opcode::Add;
        case BinaryOp::Sub:    return Opcode::Sub;
        case BinaryOp::Mul:    return Opcode::Mul;
        case BinaryOp::Div:    return Opcode::Div;
        case BinaryOp::Mod:    return Opcode::Mod;
        case BinaryOp::Eq:     return Opcode::CmpEq;
        case BinaryOp::NotEq:  return Opcode::CmpNe;
        case BinaryOp::Lt:     return Opcode::CmpLt;
        case BinaryOp::Gt:     return Opcode::CmpGt;
        case BinaryOp::LtEq:   return Opcode::CmpLe;
        case BinaryOp::GtEq:   return Opcode::CmpGe;
        case BinaryOp::And:    return Opcode::LogAnd;
        case BinaryOp::Or:     return Opcode::LogOr;
        case BinaryOp::BitAnd: return Opcode::BitAnd;
        case BinaryOp::BitOr:  return Opcode::BitOr;
        case BinaryOp::BitXor: return Opcode::BitXor;
        case BinaryOp::Shl:    return Opcode::Shl;
        case BinaryOp::Shr:    return Opcode::Shr;
    }
    return Opcode::Add;  // unreachable
}

Opcode IRGen::unaryOpToOpcode(UnaryOp op) const noexcept {
    switch (op) {
        case UnaryOp::Neg:    return Opcode::Neg;
        case UnaryOp::Not:    return Opcode::LogNot;
        case UnaryOp::BitNot: return Opcode::BitNot;
        default:              return Opcode::Copy;  // Deref / AddrOf: handled separately
    }
}

// ─── Module ───────────────────────────────────────────────────────────────────

void IRGen::visit(ModuleDecl& m) {
    for (auto& d : m.decls()) d->accept(*this);
}

// ─── Function ────────────────────────────────────────────────────────────────
//
// Calling convention
// ──────────────────
// Each formal parameter gets:
//   1. An alloca slot  (x = alloca)          so the body can load/store it.
//   2. A param instr   (t1 = param 0)        that retrieves the incoming value.
//   3. A store         (store t1 x)          that initialises the slot.
//
// This makes parameters behave identically to local variables throughout
// the rest of code-generation.

void IRGen::visit(FunctionDecl& fn) {
    IRFunction& irFn = module_->addFunction(fn.name());
    auto& entry = irFn.addBlock("entry");
    builder_.reset(irFn, entry);
    varSlots_.clear();

    for (uint32_t i = 0; i < static_cast<uint32_t>(fn.params().size()); ++i) {
        const auto& p  = *fn.params()[i];
        RegID slot     = builder_.buildAlloca(p.name());  // x = alloca
        RegID incoming = builder_.buildParam(i);          // t1 = param 0
        builder_.buildStore(incoming, slot);              // store t1 x
        varSlots_[p.name()] = slot;
    }

    if (fn.body()) const_cast<Stmt*>(fn.body())->accept(*this);

    if (!builder_.isTerminated())
        builder_.buildRet();

    builder_.reset();
}

// ─── Variable declaration ─────────────────────────────────────────────────────
//
// `let z = x + y`  emits:
//   z  = alloca
//   tN = <evaluate initializer>   (e.g. t3 = add t1 t2)
//   store tN z

void IRGen::visit(VarDecl& v) {
    RegID slot = builder_.buildAlloca(v.name());
    varSlots_[v.name()] = slot;

    if (v.initializer()) {
        const_cast<Expr*>(v.initializer())->accept(*this);
        builder_.buildStore(lastReg_, slot);
    }
}

// ─── Statements ───────────────────────────────────────────────────────────────

void IRGen::visit(BlockStmt& b) {
    for (auto& s : b.stmts()) s->accept(*this);
}

void IRGen::visit(ExprStmt& s) {
    s.expr().accept(*this);
}

void IRGen::visit(ReturnStmt& r) {
    if (r.value()) {
        const_cast<Expr*>(r.value())->accept(*this);
        builder_.buildRet(lastReg_);
    } else {
        builder_.buildRet();
    }
    // Start an unreachable block for any code following this return.
    auto& dead = builder_.addBlock(builder_.newLabel("after_ret"));
    builder_.setBlock(dead);
}

void IRGen::visit(IfStmt& s) {
    const_cast<Expr&>(s.condition()).accept(*this);
    const RegID cond = lastReg_;

    const std::string thenLabel  = builder_.newLabel("then");
    const std::string elseLabel  = s.elseBranch() ? builder_.newLabel("else") : "";
    const std::string mergeLabel = builder_.newLabel("merge");

    builder_.buildCondBranch(cond, thenLabel,
                              s.elseBranch() ? elseLabel : mergeLabel);

    // then-block
    builder_.setBlock(builder_.addBlock(thenLabel));
    const_cast<Stmt&>(s.thenBranch()).accept(*this);
    if (!builder_.isTerminated())
        builder_.buildBranch(mergeLabel);

    // else-block
    if (s.elseBranch()) {
        builder_.setBlock(builder_.addBlock(elseLabel));
        const_cast<Stmt*>(s.elseBranch())->accept(*this);
        if (!builder_.isTerminated())
            builder_.buildBranch(mergeLabel);
    }

    // merge-block
    builder_.setBlock(builder_.addBlock(mergeLabel));
}

void IRGen::visit(WhileStmt& s) {
    const std::string condLabel = builder_.newLabel("while_cond");
    const std::string bodyLabel = builder_.newLabel("while_body");
    const std::string exitLabel = builder_.newLabel("while_exit");

    builder_.buildBranch(condLabel);

    builder_.setBlock(builder_.addBlock(condLabel));
    const_cast<Expr&>(s.condition()).accept(*this);
    builder_.buildCondBranch(lastReg_, bodyLabel, exitLabel);

    builder_.setBlock(builder_.addBlock(bodyLabel));
    const_cast<Stmt&>(s.body()).accept(*this);
    if (!builder_.isTerminated())
        builder_.buildBranch(condLabel);

    builder_.setBlock(builder_.addBlock(exitLabel));
}

void IRGen::visit(VloopStmt& s) {
    const std::string condLabel = builder_.newLabel("vloop_cond");
    const std::string bodyLabel = builder_.newLabel("vloop_body");
    const std::string exitLabel = builder_.newLabel("vloop_exit");

    builder_.buildBranch(condLabel);

    builder_.setBlock(builder_.addBlock(condLabel));
    const_cast<Expr&>(s.condition()).accept(*this);
    builder_.buildCondBranch(lastReg_, bodyLabel, exitLabel);

    builder_.setBlock(builder_.addBlock(bodyLabel));
    const_cast<Stmt&>(s.body()).accept(*this);
    if (!builder_.isTerminated())
        builder_.buildBranch(condLabel);

    builder_.setBlock(builder_.addBlock(exitLabel));
}

void IRGen::visit(MatchStmt& s) {
    // Lower: evaluate subject, then chain of cmp + condbranch per arm.
    const_cast<Expr&>(s.subject()).accept(*this);
    RegID subj = lastReg_;

    const std::string exitLabel = builder_.newLabel("match_exit");

    for (std::size_t i = 0; i < s.arms().size(); ++i) {
        auto& arm = s.arms()[i];
        const std::string bodyLabel = builder_.newLabel("match_arm");
        const std::string nextLabel = (i + 1 < s.arms().size())
            ? builder_.newLabel("match_next")
            : exitLabel;

        if (!arm.pattern) {
            // wildcard '_' — unconditional branch to body
            builder_.buildBranch(bodyLabel);
        } else {
            const_cast<Expr&>(*arm.pattern).accept(*this);
            RegID patternReg = lastReg_;
            RegID cmpReg = builder_.buildBinOp(Opcode::CmpEq, subj, patternReg);
            builder_.buildCondBranch(cmpReg, bodyLabel, nextLabel);
        }

        builder_.setBlock(builder_.addBlock(bodyLabel));
        const_cast<Stmt&>(*arm.body).accept(*this);
        if (!builder_.isTerminated())
            builder_.buildBranch(exitLabel);

        if (!arm.pattern) break; // wildcard was last, no next block needed
        builder_.setBlock(builder_.addBlock(nextLabel));
    }

    // Ensure we land in exit block
    if (!builder_.isTerminated())
        builder_.buildBranch(exitLabel);
    builder_.setBlock(builder_.addBlock(exitLabel));
}

void IRGen::visit(ArrayLiteralExpr& e) {
    // Allocate a slot for each element and store it; return the slot of element 0
    // as a representative register. Full heap allocation deferred to runtime.
    RegID firstSlot = 0;
    for (std::size_t i = 0; i < e.elements().size(); ++i) {
        const_cast<Expr&>(*e.elements()[i]).accept(*this);
        std::string slotName = "__arr_" + std::to_string(i);
        RegID slot = builder_.buildAlloca(slotName);
        builder_.buildStore(lastReg_, slot);
        if (i == 0) firstSlot = slot;
    }
    lastReg_ = firstSlot;
}

void IRGen::visit(ForStmt& s) {
    RegID slot = builder_.buildAlloca(s.variable());
    varSlots_[s.variable()] = slot;

    const_cast<Expr&>(s.iterable()).accept(*this);
    RegID iterSlot = builder_.buildAlloca("__iter");
    builder_.buildStore(lastReg_, iterSlot);

    const std::string condLabel = builder_.newLabel("for_cond");
    const std::string bodyLabel = builder_.newLabel("for_body");
    const std::string exitLabel = builder_.newLabel("for_exit");

    builder_.buildBranch(condLabel);

    builder_.setBlock(builder_.addBlock(condLabel));
    RegID iterCur = builder_.buildLoad(iterSlot);
    builder_.buildCondBranch(iterCur, bodyLabel, exitLabel);

    builder_.setBlock(builder_.addBlock(bodyLabel));
    const_cast<Stmt&>(s.body()).accept(*this);
    if (!builder_.isTerminated())
        builder_.buildBranch(condLabel);

    builder_.setBlock(builder_.addBlock(exitLabel));
}

// ─── Expressions: literals ────────────────────────────────────────────────────

void IRGen::visit(IntLiteralExpr&    e) { lastReg_ = builder_.buildInt(e.value()); }
void IRGen::visit(FloatLiteralExpr&  e) { lastReg_ = builder_.buildFloat(e.value()); }
void IRGen::visit(BoolLiteralExpr&   e) { lastReg_ = builder_.buildBool(e.value()); }
void IRGen::visit(StringLiteralExpr& e) { lastReg_ = builder_.buildString(e.value()); }
void IRGen::visit(CharLiteralExpr&   e) { lastReg_ = builder_.buildChar(e.value()); }
void IRGen::visit(NilLiteralExpr&)      { lastReg_ = builder_.buildNil(); }

// ─── Expressions: identifiers ─────────────────────────────────────────────────
//
// `x` where x has an alloca slot  →  tN = load x
// `x` where x has no slot         →  tN = load x   (symbolic / global)

void IRGen::visit(IdentExpr& e) {
    auto it = varSlots_.find(e.name());
    if (it == varSlots_.end())
        lastReg_ = builder_.buildLoadNamed(e.name());
    else
        lastReg_ = builder_.buildLoad(it->second);
}

// ─── Expressions: binary ─────────────────────────────────────────────────────
//
// `x + y`  →  t1 = load x; t2 = load y; t3 = add t1 t2

void IRGen::visit(BinaryExpr& e) {
    e.lhs().accept(*this);
    const RegID lhs = lastReg_;
    e.rhs().accept(*this);
    const RegID rhs = lastReg_;
    lastReg_ = builder_.buildBinOp(binaryOpToOpcode(e.op()), lhs, rhs);
}

// ─── Expressions: unary ───────────────────────────────────────────────────────

void IRGen::visit(UnaryExpr& e) {
    e.operand().accept(*this);
    const RegID operand = lastReg_;

    switch (e.op()) {
        case UnaryOp::Deref:
            // *ptr → load from the pointer value
            lastReg_ = builder_.buildLoad(operand);
            break;
        case UnaryOp::AddrOf:
            // &var → the alloca register IS the address; pass it through
            lastReg_ = operand;
            break;
        default:
            lastReg_ = builder_.buildUnOp(unaryOpToOpcode(e.op()), operand);
            break;
    }
}

// ─── Expressions: assignment ─────────────────────────────────────────────────
//
// Simple assignment: `z = expr`  →  tN = <eval expr>; store tN z
// Compound:         `z += expr`  →  tL = load z; tR = <eval expr>;
//                                   tN = add tL tR; store tN z

void IRGen::visit(AssignExpr& e) {
    RegID val = NoReg;

    if (e.op() == AssignOp::Assign) {
        const_cast<Expr&>(e.value()).accept(*this);
        val = lastReg_;
    } else {
        // Compound assignment: lhs op= rhs  →  lhs = lhs op rhs
        const_cast<Expr&>(e.target()).accept(*this);   // load lhs
        const RegID lhsVal = lastReg_;
        const_cast<Expr&>(e.value()).accept(*this);    // eval rhs
        const RegID rhsVal = lastReg_;

        Opcode binOp = Opcode::Add;
        switch (e.op()) {
            case AssignOp::AddAssign:    binOp = Opcode::Add;    break;
            case AssignOp::SubAssign:    binOp = Opcode::Sub;    break;
            case AssignOp::MulAssign:    binOp = Opcode::Mul;    break;
            case AssignOp::DivAssign:    binOp = Opcode::Div;    break;
            case AssignOp::ModAssign:    binOp = Opcode::Mod;    break;
            case AssignOp::BitAndAssign: binOp = Opcode::BitAnd; break;
            case AssignOp::BitOrAssign:  binOp = Opcode::BitOr;  break;
            case AssignOp::BitXorAssign: binOp = Opcode::BitXor; break;
            default: break;
        }
        val = builder_.buildBinOp(binOp, lhsVal, rhsVal);
    }

    if (const auto* ident = dynamic_cast<const IdentExpr*>(&e.target())) {
        auto it = varSlots_.find(ident->name());
        if (it != varSlots_.end())
            builder_.buildStore(val, it->second);
    }
    // Future: MemberExpr / IndexExpr assignment targets.
    lastReg_ = val;
}

// ─── Expressions: member access ───────────────────────────────────────────────

void IRGen::visit(MemberExpr& e) {
    const_cast<Expr&>(e.object()).accept(*this);
    // Phase 3: replace with GEP once struct layouts are known.
    // For now, emit a symbolic load annotated with the field name.
    lastReg_ = builder_.buildLoadNamed(e.member());
}

// ─── Expressions: index ───────────────────────────────────────────────────────

void IRGen::visit(IndexExpr& e) {
    const_cast<Expr&>(e.base()).accept(*this);
    const RegID base = lastReg_;
    const_cast<Expr&>(e.index()).accept(*this);
    const RegID idx = lastReg_;
    // Phase 3: GEP + load.  For now, compute (base + idx) as a placeholder.
    lastReg_ = builder_.buildBinOp(Opcode::Add, base, idx);
}

// ─── Expressions: cast ────────────────────────────────────────────────────────

void IRGen::visit(CastExpr& e) {
    const_cast<Expr&>(e.expr()).accept(*this);
    const RegID val = lastReg_;
    // Phase 3: use the actual target-type name from the TypeNode.
    lastReg_ = builder_.buildCast(val, "?");
}

// ─── Expressions: call ───────────────────────────────────────────────────────
//
// `add(x, y)`  →  t1 = load x; t2 = load y; tN = call add t1 t2

void IRGen::visit(CallExpr& e) {
    std::string calleeName;
    if (const auto* ident = dynamic_cast<const IdentExpr*>(&e.callee())) {
        calleeName = ident->name();
    } else {
        const_cast<Expr&>(e.callee()).accept(*this);
        calleeName = "%" + std::to_string(lastReg_);  // indirect call
    }

    std::vector<RegID> args;
    args.reserve(e.args().size());
    for (auto& arg : e.args()) {
        const_cast<Expr&>(*arg).accept(*this);
        args.push_back(lastReg_);
    }

    lastReg_ = builder_.buildCall(std::move(calleeName), args);
}


} // namespace vcc::ir
