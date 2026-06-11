#pragma once

#include "ASTNode.h"
#include "ASTVisitor.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/// @file Expressions.h
/// Expression AST nodes for the V language.
///
/// Hierarchy
/// ─────────
///   Expr
///   ├── BinaryExpr        – lhs op rhs
///   ├── UnaryExpr         – op operand
///   ├── AssignExpr        – lhs = rhs  (and +=, -=, …)
///   ├── CallExpr          – callee(args…)
///   ├── IndexExpr         – base[index]
///   ├── MemberExpr        – object.field
///   ├── CastExpr          – expr as Type
///   ├── IdentExpr         – bare identifier
///   ├── IntLiteralExpr    – 42
///   ├── FloatLiteralExpr  – 3.14
///   ├── StringLiteralExpr – "hello"
///   ├── CharLiteralExpr   – 'c'
///   ├── BoolLiteralExpr   – true / false
///   └── NilLiteralExpr    – nil
///
/// Future extensions
/// ─────────────────
///  • StructLiteralExpr: `Point { x: 1, y: 2 }`
///  • MatchExpr: `match val { … }`
///  • ClosureExpr: `|x| x + 1`
///  • RangeExpr: `0..10`

namespace vcc::ast {

// ─── Binary operators ─────────────────────────────────────────────────────────

enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,        // arithmetic
    Eq, NotEq, Lt, Gt, LtEq, GtEq, // comparison
    And, Or,                         // logical
    BitAnd, BitOr, BitXor,           // bitwise
    Shl, Shr,                        // shifts
};

[[nodiscard]] std::string_view binaryOpName(BinaryOp op) noexcept;

// ─── Unary operators ──────────────────────────────────────────────────────────

enum class UnaryOp {
    Neg,    ///< -x
    Not,    ///< !x
    BitNot, ///< ~x
    Deref,  ///< *x
    AddrOf, ///< &x
};

[[nodiscard]] std::string_view unaryOpName(UnaryOp op) noexcept;

// ─── Assignment operators ─────────────────────────────────────────────────────

enum class AssignOp {
    Assign,          ///< =
    AddAssign,       ///< +=
    SubAssign,       ///< -=
    MulAssign,       ///< *=
    DivAssign,       ///< /=
    ModAssign,       ///< %=
    BitAndAssign,    ///< &=
    BitOrAssign,     ///< |=
    BitXorAssign,    ///< ^=
};

// ─── BinaryExpr ───────────────────────────────────────────────────────────────

class BinaryExpr : public Expr {
public:
    BinaryExpr(BinaryOp             op,
               std::unique_ptr<Expr> lhs,
               std::unique_ptr<Expr> rhs,
               common::SourceRange  range = {})
        : Expr(range), op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    [[nodiscard]] BinaryOp   op()  const noexcept { return op_; }
    [[nodiscard]] const Expr& lhs() const noexcept { return *lhs_; }
    [[nodiscard]] const Expr& rhs() const noexcept { return *rhs_; }
    [[nodiscard]] Expr&       lhs()       noexcept { return *lhs_; }
    [[nodiscard]] Expr&       rhs()       noexcept { return *rhs_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "BinaryExpr"; }

private:
    BinaryOp              op_;
    std::unique_ptr<Expr> lhs_;
    std::unique_ptr<Expr> rhs_;
};

// ─── UnaryExpr ────────────────────────────────────────────────────────────────

class UnaryExpr : public Expr {
public:
    UnaryExpr(UnaryOp              op,
              std::unique_ptr<Expr> operand,
              common::SourceRange  range = {})
        : Expr(range), op_(op), operand_(std::move(operand)) {}

    [[nodiscard]] UnaryOp   op()      const noexcept { return op_; }
    [[nodiscard]] const Expr& operand() const noexcept { return *operand_; }
    [[nodiscard]] Expr&       operand()       noexcept { return *operand_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "UnaryExpr"; }

private:
    UnaryOp               op_;
    std::unique_ptr<Expr> operand_;
};

// ─── AssignExpr ───────────────────────────────────────────────────────────────

class AssignExpr : public Expr {
public:
    AssignExpr(AssignOp             op,
               std::unique_ptr<Expr> target,
               std::unique_ptr<Expr> value,
               common::SourceRange  range = {})
        : Expr(range), op_(op), target_(std::move(target)), value_(std::move(value)) {}

    [[nodiscard]] AssignOp   op()     const noexcept { return op_; }
    [[nodiscard]] const Expr& target() const noexcept { return *target_; }
    [[nodiscard]] const Expr& value()  const noexcept { return *value_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "AssignExpr"; }

private:
    AssignOp              op_;
    std::unique_ptr<Expr> target_;
    std::unique_ptr<Expr> value_;
};

// ─── CallExpr ─────────────────────────────────────────────────────────────────

class CallExpr : public Expr {
public:
    CallExpr(std::unique_ptr<Expr>              callee,
             std::vector<std::unique_ptr<Expr>> args,
             common::SourceRange                range = {})
        : Expr(range), callee_(std::move(callee)), args_(std::move(args)) {}

    [[nodiscard]] const Expr& callee() const noexcept { return *callee_; }
    [[nodiscard]] const std::vector<std::unique_ptr<Expr>>& args() const noexcept { return args_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "CallExpr"; }

private:
    std::unique_ptr<Expr>              callee_;
    std::vector<std::unique_ptr<Expr>> args_;
};

// ─── IndexExpr ────────────────────────────────────────────────────────────────

class IndexExpr : public Expr {
public:
    IndexExpr(std::unique_ptr<Expr> base,
              std::unique_ptr<Expr> index,
              common::SourceRange  range = {})
        : Expr(range), base_(std::move(base)), index_(std::move(index)) {}

    [[nodiscard]] const Expr& base()  const noexcept { return *base_; }
    [[nodiscard]] const Expr& index() const noexcept { return *index_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "IndexExpr"; }

private:
    std::unique_ptr<Expr> base_;
    std::unique_ptr<Expr> index_;
};

// ─── MemberExpr ───────────────────────────────────────────────────────────────

class MemberExpr : public Expr {
public:
    MemberExpr(std::unique_ptr<Expr> object,
               std::string          member,
               common::SourceRange  range = {})
        : Expr(range), object_(std::move(object)), member_(std::move(member)) {}

    [[nodiscard]] const Expr&   object() const noexcept { return *object_; }
    [[nodiscard]] const std::string& member() const noexcept { return member_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "MemberExpr"; }

private:
    std::unique_ptr<Expr> object_;
    std::string           member_;
};

// ─── CastExpr ────────────────────────────────────────────────────────────────

class CastExpr : public Expr {
public:
    CastExpr(std::unique_ptr<Expr>     expr,
             std::unique_ptr<TypeNode> targetType,
             common::SourceRange       range = {})
        : Expr(range), expr_(std::move(expr)), targetType_(std::move(targetType)) {}

    [[nodiscard]] const Expr&     expr()       const noexcept { return *expr_; }
    [[nodiscard]] const TypeNode& targetType() const noexcept { return *targetType_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "CastExpr"; }

private:
    std::unique_ptr<Expr>     expr_;
    std::unique_ptr<TypeNode> targetType_;
};

// ─── IdentExpr / IdentifierExpr ─────────────────────────────────────────────

class IdentExpr : public Expr {
public:
    explicit IdentExpr(std::string name, common::SourceRange range = {})
        : Expr(range), name_(std::move(name)) {}

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "IdentExpr"; }

private:
    std::string name_;
};

/// Canonical alias used in requirement-spec names.
using IdentifierExpr = IdentExpr;

// ─── LiteralExpr – abstract base for all literal value expressions ───────────

/// Abstract intermediate base class for every literal.  Provides a single
/// type to test with `dynamic_cast<LiteralExpr*>()` and a hook for visitor
/// passes that want to handle all literals uniformly.
///
///   LiteralExpr
///   ├── IntLiteralExpr    – 42, 0xFF, 0b1010, 0o77
///   ├── FloatLiteralExpr  – 3.14, 1.0e-9
///   ├── StringLiteralExpr – "hello"
///   ├── CharLiteralExpr   – 'a'
///   ├── BoolLiteralExpr   – true / false
///   └── NilLiteralExpr    – nil
class LiteralExpr : public Expr {
public:
    using Expr::Expr;
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "LiteralExpr"; }
    // accept() is still abstract — concrete subclasses implement it.
};

class IntLiteralExpr : public LiteralExpr {
public:
    explicit IntLiteralExpr(int64_t value, common::SourceRange range = {})
        : LiteralExpr(range), value_(value) {}
    [[nodiscard]] int64_t value() const noexcept { return value_; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "IntLiteralExpr"; }
private:
    int64_t value_;
};

class FloatLiteralExpr : public LiteralExpr {
public:
    explicit FloatLiteralExpr(double value, common::SourceRange range = {})
        : LiteralExpr(range), value_(value) {}
    [[nodiscard]] double value() const noexcept { return value_; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "FloatLiteralExpr"; }
private:
    double value_;
};

class StringLiteralExpr : public LiteralExpr {
public:
    explicit StringLiteralExpr(std::string value, common::SourceRange range = {})
        : LiteralExpr(range), value_(std::move(value)) {}
    [[nodiscard]] const std::string& value() const noexcept { return value_; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "StringLiteralExpr"; }
private:
    std::string value_;
};

class CharLiteralExpr : public LiteralExpr {
public:
    explicit CharLiteralExpr(char32_t value, common::SourceRange range = {})
        : LiteralExpr(range), value_(value) {}
    [[nodiscard]] char32_t value() const noexcept { return value_; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "CharLiteralExpr"; }
private:
    char32_t value_;
};

class BoolLiteralExpr : public LiteralExpr {
public:
    explicit BoolLiteralExpr(bool value, common::SourceRange range = {})
        : LiteralExpr(range), value_(value) {}
    [[nodiscard]] bool value() const noexcept { return value_; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "BoolLiteralExpr"; }
private:
    bool value_;
};

class NilLiteralExpr : public LiteralExpr {
public:
    explicit NilLiteralExpr(common::SourceRange range = {}) : LiteralExpr(range) {}
    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "NilLiteralExpr"; }
};

} // namespace vcc::ast
