#pragma once

#include "SymbolTable.h"
#include "TypeChecker.h"
#include "vcc/ast/ASTVisitor.h"
#include "vcc/ast/Declarations.h"
#include "vcc/ast/Expressions.h"
#include "vcc/ast/Statements.h"
#include "vcc/common/CompilerContext.h"

/// @file SemanticAnalyzer.h
/// Orchestrates all semantic analysis passes for VCC.
///
/// Passes (in order)
/// ─────────────────
///  1. Name resolution    – populate & query the symbol table.
///  2. Type checking      – verify type correctness (TypeChecker).
///
/// Future passes
/// ─────────────
///  • Borrow checker / ownership analysis.
///  • Constant folding (ConstantEvaluator).
///  • Dead-code analysis.
///  • Unused-variable / unused-import warnings.
///  • Exhaustiveness checking for match expressions.

namespace vcc::semantic {

class SemanticAnalyzer : public ast::ASTVisitorBase {
public:
    explicit SemanticAnalyzer(common::CompilerContext& ctx);

    /// Run all semantic passes on a module.
    /// @returns true iff no errors were emitted.
    [[nodiscard]] bool analyze(ast::ModuleDecl& module);

    // ── Name resolution pass ─────────────────────────────────────────────────

    void visit(ast::ModuleDecl&)      override;
    void visit(ast::ImportDecl&)      override;
    void visit(ast::FunctionDecl&)    override;
    void visit(ast::ParameterDecl&)   override;
    void visit(ast::StructDecl&)      override;
    void visit(ast::EnumDecl&)        override;
    void visit(ast::VarDecl&)         override;
    void visit(ast::TypeAliasDecl&)   override;

    void visit(ast::BlockStmt&)       override;
    void visit(ast::IfStmt&)          override;
    void visit(ast::WhileStmt&)       override;
    void visit(ast::VloopStmt&)       override;
    void visit(ast::ForStmt&)         override;
    void visit(ast::ReturnStmt&)      override;
    void visit(ast::ExprStmt&)        override;

    void visit(ast::IdentExpr&)       override;
    void visit(ast::CallExpr&)        override;
    void visit(ast::BinaryExpr&)      override;
    void visit(ast::UnaryExpr&)       override;
    void visit(ast::AssignExpr&)      override;
    void visit(ast::MemberExpr&)      override;

private:
    common::CompilerContext& ctx_;
    SymbolTable              symbols_;

    /// Name of the function currently being analysed (for return-type checks).
    std::string currentFunction_;
};

} // namespace vcc::semantic
