#pragma once

#include "SymbolTable.h"
#include "vcc/ast/ASTVisitor.h"
#include "vcc/ast/Declarations.h"
#include "vcc/ast/Expressions.h"
#include "vcc/ast/Statements.h"
#include "vcc/common/CompilerContext.h"

/// @file TypeChecker.h
/// Type-checking pass for the V language.
///
/// The TypeChecker is an ASTVisitor that:
///  1. Infers types for expressions.
///  2. Verifies that operand types match operator constraints.
///  3. Checks that function call argument types match parameter types.
///  4. Ensures return types match function signatures.
///
/// It runs after the SemanticAnalyzer has resolved all names.
///
/// Future extensions
/// ─────────────────
///  • A proper Type object hierarchy (instead of string names).
///  • Hindley-Milner style bidirectional type inference.
///  • Generic type instantiation.
///  • Implicit coercions (integer widening, &T → *T, etc.)

namespace vcc::semantic {

class TypeChecker : public ast::ASTVisitorBase {
public:
    TypeChecker(common::CompilerContext& ctx, SymbolTable& symbols);

    /// Entry point: type-check a full module.
    void check(ast::ModuleDecl& module);

    // ── Declarations ────────────────────────────────────────────────────────
    void visit(ast::ModuleDecl&)      override;
    void visit(ast::FunctionDecl&)    override;
    void visit(ast::ParameterDecl&)   override;
    void visit(ast::StructDecl&)      override;
    void visit(ast::VarDecl&)         override;

    // ── Statements ──────────────────────────────────────────────────────────
    void visit(ast::BlockStmt&)    override;
    void visit(ast::ReturnStmt&)   override;
    void visit(ast::IfStmt&)       override;
    void visit(ast::WhileStmt&)    override;
    void visit(ast::VloopStmt&)    override;
    void visit(ast::MatchStmt&)     override;
    void visit(ast::ArrayLiteralExpr&) override;
    void visit(ast::ForStmt&)      override;
    void visit(ast::ExprStmt&)     override;

    // ── Expressions ─────────────────────────────────────────────────────────
    void visit(ast::BinaryExpr&)       override;
    void visit(ast::UnaryExpr&)        override;
    void visit(ast::CallExpr&)         override;
    void visit(ast::IdentExpr&)        override;
    void visit(ast::AssignExpr&)       override;
    void visit(ast::IntLiteralExpr&)   override;
    void visit(ast::FloatLiteralExpr&) override;
    void visit(ast::BoolLiteralExpr&)  override;
    void visit(ast::StringLiteralExpr&)override;

private:
    /// Return the inferred type name of the most recently visited expression.
    /// This is a simplistic string-based scheme; Phase 3 replaces it with
    /// a proper Type* from the type system.
    [[nodiscard]] const std::string& exprType() const noexcept;
    void setExprType(std::string t);

    bool typesCompatible(const std::string& a, const std::string& b) const noexcept;

    common::CompilerContext& ctx_;
    SymbolTable&             symbols_;
    std::string              currentReturnType_;  ///< Expected return type of current fn
    std::string              lastExprType_;       ///< Result of last expression visit
    bool                     hasReturn_{false};     ///< Did the current fn body have a return?
    bool                     requiresReturn_{false}; ///< Does the current fn declare a return type?
};

} // namespace vcc::semantic
