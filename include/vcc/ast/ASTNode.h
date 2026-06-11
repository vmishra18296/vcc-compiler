#pragma once

#include "vcc/common/SourceLocation.h"

#include <memory>
#include <string>

/// @file ASTNode.h
/// Root of the VCC abstract syntax tree node hierarchy.
///
/// Ownership model
/// ───────────────
///  Every AST node is heap-allocated and owned exclusively by its parent
///  via std::unique_ptr.  The ModuleDecl at the root owns everything.
///
/// Visitor integration
/// ───────────────────
///  Every concrete node overrides `accept(ASTVisitor&)`.  Two visitors are
///  provided out-of-the-box: ASTDumper (textual dump) and the no-op
///  ASTVisitorBase (selective override).
///
/// Node categories
/// ───────────────
///   ASTNode
///   ├── Decl   (declarations.h)
///   ├── Stmt   (statements.h)
///   ├── Expr   (expressions.h)
///   └── TypeNode (types.h)
///
/// Future extensions
/// ─────────────────
///  • Attach semantic type information via a `resolvedType` pointer set
///    during the type-checking phase.
///  • Parent-pointer back-links for refactoring / navigation tools.
///  • Source-comment attachment for doc-comment generation.

namespace vcc::ast {

class ASTVisitor;  // Forward declaration

// ─── ASTNode base ─────────────────────────────────────────────────────────────

class ASTNode {
public:
    explicit ASTNode(common::SourceRange range = {}) : range_(range) {}
    virtual ~ASTNode() = default;

    // Non-copyable; unique ownership
    ASTNode(const ASTNode&)            = delete;
    ASTNode& operator=(const ASTNode&) = delete;
    ASTNode(ASTNode&&)                 = default;
    ASTNode& operator=(ASTNode&&)      = default;

    /// Source range covering this node (used for diagnostics and IDE features).
    [[nodiscard]] const common::SourceRange& range() const noexcept { return range_; }
    void setRange(common::SourceRange r) noexcept { range_ = r; }

    /// Visitor dispatch.  Every concrete node must override this.
    virtual void accept(ASTVisitor& visitor) = 0;

    /// Short name of this node kind (for debugging / dump utilities).
    [[nodiscard]] virtual std::string_view nodeName() const noexcept = 0;

protected:
    common::SourceRange range_;
};

// ─── Category base classes ────────────────────────────────────────────────────

/// Base for all declaration nodes (functions, variables, structs, …).
class Decl : public ASTNode {
public:
    using ASTNode::ASTNode;
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "Decl"; }
};

/// Base for all statement nodes (if, while, return, expression-stmt, …).
class Stmt : public ASTNode {
public:
    using ASTNode::ASTNode;
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "Stmt"; }
};

/// Base for all expression nodes (binary ops, calls, literals, …).
class Expr : public ASTNode {
public:
    using ASTNode::ASTNode;
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "Expr"; }
};

/// Base for all type-annotation nodes (primitive types, user types, refs, …).
class TypeNode : public ASTNode {
public:
    using ASTNode::ASTNode;
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "TypeNode"; }
};

// ─── Canonical name aliases ───────────────────────────────────────────────────
// These let user-facing code and documentation use the requirement-spec names
// without any runtime overhead.

/// Alias: ASTNode is the root "Node" in the hierarchy.
using Node      = ASTNode;
/// Alias: Stmt is the "Statement" base.
using Statement = Stmt;
/// Alias: Expr is the "Expression" base.
using Expression = Expr;

} // namespace vcc::ast
