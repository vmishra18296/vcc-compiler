#pragma once

#include "IRBuilder.h"
#include "IRModule.h"
#include "vcc/ast/ASTVisitor.h"
#include "vcc/ast/Declarations.h"
#include "vcc/ast/Expressions.h"
#include "vcc/ast/Statements.h"
#include "vcc/common/CompilerContext.h"

#include <unordered_map>

/// @file IRGen.h
/// Lowers a type-checked VCC AST to VCC IR.
///
/// Design
/// ──────
///  IRGen is an ASTVisitor that walks the AST in a single pass and emits
///  IRInstructions into IRBasicBlocks inside IRFunctions inside an IRModule.
///
///  All instruction emission is delegated to IRBuilder, which owns the
///  current insert-point (function + block) and handles register allocation.
///
///  Expressions return their result via `lastReg_` so that parent nodes can
///  read the result without back-patching.
///
/// Calling convention
/// ──────────────────
///  Each formal parameter gets an alloca slot, a `param N` instruction, and
///  a store that initialises the slot from the incoming argument value.  This
///  means the body always accesses parameters via load/store — exactly the
///  same pattern used for local variables — which simplifies optimisation.
///
///  Example: `fn add(x: i64, y: i64) -> i64 { return x + y }`
///  @code
///    entry:
///      x  = alloca
///      t1 = param 0
///      store t1 x
///      y  = alloca
///      t2 = param 1
///      store t2 y
///      t3 = load x
///      t4 = load y
///      t5 = add t3 t4
///      ret t5
///  @endcode

namespace vcc::ir {

class IRGen : public ast::ASTVisitorBase {
public:
    explicit IRGen(common::CompilerContext& ctx);

    /// Entry point: lower a module and return its IR.
    [[nodiscard]] std::unique_ptr<IRModule> generate(ast::ModuleDecl& module);

    // ── Declarations ──────────────────────────────────────────────────────────
    void visit(ast::ModuleDecl&)   override;
    void visit(ast::FunctionDecl&) override;
    void visit(ast::VarDecl&)      override;

    // ── Statements ────────────────────────────────────────────────────────────
    void visit(ast::BlockStmt&)    override;
    void visit(ast::ExprStmt&)     override;
    void visit(ast::ReturnStmt&)   override;
    void visit(ast::IfStmt&)       override;
    void visit(ast::WhileStmt&)    override;
    void visit(ast::VloopStmt&)    override;
    void visit(ast::ForStmt&)      override;

    // ── Expressions ───────────────────────────────────────────────────────────
    void visit(ast::BinaryExpr&)        override;
    void visit(ast::UnaryExpr&)         override;
    void visit(ast::AssignExpr&)        override;
    void visit(ast::CallExpr&)          override;
    void visit(ast::IdentExpr&)         override;
    void visit(ast::MemberExpr&)        override;
    void visit(ast::IndexExpr&)         override;
    void visit(ast::CastExpr&)          override;
    void visit(ast::IntLiteralExpr&)    override;
    void visit(ast::FloatLiteralExpr&)  override;
    void visit(ast::BoolLiteralExpr&)   override;
    void visit(ast::StringLiteralExpr&) override;
    void visit(ast::CharLiteralExpr&)   override;
    void visit(ast::NilLiteralExpr&)    override;

private:
    // ── Helpers ───────────────────────────────────────────────────────────────

    /// Map a BinaryOp to an IR Opcode.
    [[nodiscard]] Opcode binaryOpToOpcode(ast::BinaryOp op) const noexcept;

    /// Map a UnaryOp to an IR Opcode.
    [[nodiscard]] Opcode unaryOpToOpcode(ast::UnaryOp op) const noexcept;

    // ── State ─────────────────────────────────────────────────────────────────
    common::CompilerContext&               ctx_;
    std::unique_ptr<IRModule>             module_;
    IRBuilder                             builder_;
    RegID                                 lastReg_{NoReg};

    /// Maps variable names to their alloca-slot register.
    std::unordered_map<std::string, RegID> varSlots_;
};

} // namespace vcc::ir
