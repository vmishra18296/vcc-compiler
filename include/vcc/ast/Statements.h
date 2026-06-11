#pragma once

#include "ASTNode.h"
#include "ASTVisitor.h"
#include "Declarations.h"

#include <memory>
#include <optional>
#include <vector>

/// @file Statements.h
/// Statement AST nodes for the V language.
///
/// Hierarchy
/// ─────────
///   Stmt
///   ├── BlockStmt    – { stmts… }
///   ├── ExprStmt     – expr ;
///   ├── ReturnStmt   – return [expr]
///   ├── IfStmt       – if cond { … } [else { … }]
///   ├── WhileStmt    – while cond { … }
///   ├── ForStmt      – for var in iter { … }
///   ├── BreakStmt    – break
///   └── ContinueStmt – continue
///
/// Note: VarDecl is in Declarations.h; a VarDecl inside a function body is
/// wrapped in an ExprStmt-like position.  The parser emits a DeclStmt wrapper
/// that simply holds a VarDecl.
///
/// Future extensions
/// ─────────────────
///  • MatchStmt / MatchExpr.
///  • DeferStmt.
///  • LabeledStmt.

namespace vcc::ast {

// ─── BlockStmt ────────────────────────────────────────────────────────────────

/// A brace-delimited sequence of statements: `{ s1; s2; … }`.
class BlockStmt : public Stmt {
public:
    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts,
                       common::SourceRange                range = {})
        : Stmt(range), stmts_(std::move(stmts)) {}

    [[nodiscard]] const std::vector<std::unique_ptr<Stmt>>& stmts() const noexcept { return stmts_; }

    void addStmt(std::unique_ptr<Stmt> s) { stmts_.push_back(std::move(s)); }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "BlockStmt"; }

private:
    std::vector<std::unique_ptr<Stmt>> stmts_;
};

// ─── ExprStmt ─────────────────────────────────────────────────────────────────

/// An expression used as a statement (value is discarded).
class ExprStmt : public Stmt {
public:
    explicit ExprStmt(std::unique_ptr<Expr> expr, common::SourceRange range = {})
        : Stmt(range), expr_(std::move(expr)) {}

    [[nodiscard]] const Expr& expr() const noexcept { return *expr_; }
    [[nodiscard]] Expr&       expr()       noexcept { return *expr_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "ExprStmt"; }

private:
    std::unique_ptr<Expr> expr_;
};

// ─── DeclStmt ─────────────────────────────────────────────────────────────────

/// Wraps a VarDecl when it appears inside a block (as a statement).
/// Not exposed through the visitor directly; the parser inlines VarDecl
/// handling inside BlockStmt via a thin wrapper stored as a Stmt*.
///
/// Future: unify with the full DeclStmt once ImplDecl / local-fn are added.
class DeclStmt : public Stmt {
public:
    explicit DeclStmt(std::unique_ptr<Decl> decl, common::SourceRange range = {})
        : Stmt(range), decl_(std::move(decl)) {}

    [[nodiscard]] const Decl& decl() const noexcept { return *decl_; }
    [[nodiscard]] Decl&       decl()       noexcept { return *decl_; }

    /// Visitor dispatches to the contained decl; no separate visit() needed.
    void accept(ASTVisitor& v) override { decl_->accept(v); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "DeclStmt"; }

private:
    std::unique_ptr<Decl> decl_;
};

// ─── ReturnStmt ───────────────────────────────────────────────────────────────

class ReturnStmt : public Stmt {
public:
    explicit ReturnStmt(std::unique_ptr<Expr> value, common::SourceRange range = {})
        : Stmt(range), value_(std::move(value)) {}

    /// nullptr if bare `return` (unit return).
    [[nodiscard]] const Expr* value() const noexcept { return value_.get(); }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "ReturnStmt"; }

private:
    std::unique_ptr<Expr> value_;
};

// ─── IfStmt ───────────────────────────────────────────────────────────────────

class IfStmt : public Stmt {
public:
    IfStmt(std::unique_ptr<Expr> condition,
           std::unique_ptr<Stmt> thenBranch,
           std::unique_ptr<Stmt> elseBranch,  ///< nullptr if no else
           common::SourceRange   range = {})
        : Stmt(range),
          condition_(std::move(condition)),
          thenBranch_(std::move(thenBranch)),
          elseBranch_(std::move(elseBranch)) {}

    [[nodiscard]] const Expr& condition()  const noexcept { return *condition_; }
    [[nodiscard]] const Stmt& thenBranch() const noexcept { return *thenBranch_; }
    [[nodiscard]] const Stmt* elseBranch() const noexcept { return elseBranch_.get(); }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "IfStmt"; }

private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Stmt> thenBranch_;
    std::unique_ptr<Stmt> elseBranch_;
};

// ─── WhileStmt ────────────────────────────────────────────────────────────────

class WhileStmt : public Stmt {
public:
    WhileStmt(std::unique_ptr<Expr> condition,
              std::unique_ptr<Stmt> body,
              common::SourceRange   range = {})
        : Stmt(range), condition_(std::move(condition)), body_(std::move(body)) {}

    [[nodiscard]] const Expr& condition() const noexcept { return *condition_; }
    [[nodiscard]] const Stmt& body()      const noexcept { return *body_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "WhileStmt"; }

private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Stmt> body_;
};

// ─── VloopStmt ────────────────────────────────────────────────────────────────

/// vloop(condition) { body }  — V language loop construct
class VloopStmt : public Stmt {
public:
    VloopStmt(std::unique_ptr<Expr> condition,
              std::unique_ptr<Stmt> body,
              common::SourceRange   range = {})
        : Stmt(range), condition_(std::move(condition)), body_(std::move(body)) {}

    [[nodiscard]] const Expr& condition() const noexcept { return *condition_; }
    [[nodiscard]] const Stmt& body()      const noexcept { return *body_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "VloopStmt"; }

private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Stmt> body_;
};

// ─── ForStmt ──────────────────────────────────────────────────────────────────
class ForStmt : public Stmt {
public:
    ForStmt(std::string          variable,
            std::unique_ptr<Expr> iterable,
            std::unique_ptr<Stmt> body,
            common::SourceRange   range = {})
        : Stmt(range),
          variable_(std::move(variable)),
          iterable_(std::move(iterable)),
          body_(std::move(body)) {}

    [[nodiscard]] const std::string& variable() const noexcept { return variable_; }
    [[nodiscard]] const Expr&        iterable() const noexcept { return *iterable_; }
    [[nodiscard]] const Stmt&        body()     const noexcept { return *body_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "ForStmt"; }

private:
    std::string           variable_;
    std::unique_ptr<Expr> iterable_;
    std::unique_ptr<Stmt> body_;
};

// ─── BreakStmt / ContinueStmt ────────────────────────────────────────────────

class BreakStmt : public Stmt {
public:
    explicit BreakStmt(common::SourceRange range = {}) : Stmt(range) {}
    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "BreakStmt"; }
};

class ContinueStmt : public Stmt {
public:
    explicit ContinueStmt(common::SourceRange range = {}) : Stmt(range) {}
    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "ContinueStmt"; }
};

// ─── MatchArm / MatchStmt ────────────────────────────────────────────────────

/// A single arm of a match statement: pattern => body
struct MatchArm {
    std::unique_ptr<Expr> pattern;   ///< nullptr means wildcard '_'
    std::unique_ptr<Stmt> body;
};

/// match expr { arm* }
class MatchStmt : public Stmt {
public:
    MatchStmt(std::unique_ptr<Expr>    subject,
              std::vector<MatchArm>    arms,
              common::SourceRange      range = {})
        : Stmt(range), subject_(std::move(subject)), arms_(std::move(arms)) {}

    [[nodiscard]] const Expr&              subject() const noexcept { return *subject_; }
    [[nodiscard]] const std::vector<MatchArm>& arms() const noexcept { return arms_; }
    [[nodiscard]] std::vector<MatchArm>&   arms()    noexcept { return arms_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "MatchStmt"; }

private:
    std::unique_ptr<Expr>  subject_;
    std::vector<MatchArm>  arms_;
};

} // namespace vcc::ast
