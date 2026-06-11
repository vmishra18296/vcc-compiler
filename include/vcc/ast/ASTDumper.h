#pragma once

#include "vcc/ast/ASTVisitor.h"
#include "vcc/ast/ASTNode.h"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

/// @file ASTDumper.h
/// Visitor that prints an AST as a human-readable tree using box-drawing
/// connectors, suitable for the `--ast` compiler flag and debugging.
///
/// Output format
/// ─────────────
///   Program 'main'
///   └── FunctionDecl 'add' [pub]
///       ├── ParameterDecl 'a'
///       │   └── NamedType 'int'
///       ├── ParameterDecl 'b'
///       │   └── NamedType 'int'
///       ├── NamedType 'int'          ← return type
///       └── BlockStmt
///           └── ReturnStmt
///               └── BinaryExpr [+]
///                   ├── IdentExpr 'a'
///                   └── IdentExpr 'b'
///
/// Usage
/// ─────
///   vcc::ast::ASTDumper dumper;            // writes to std::cout
///   vcc::ast::ASTDumper dumper(myStream);  // writes to any ostream
///   dumper.dump(*moduleDecl);
///
/// Design notes
/// ────────────
///  • Inherits ASTVisitorBase so only the nodes that need output are overridden.
///  • The `prefix_` string carries the continuation characters ('│' vs spaces)
///    so each level only needs to append/pop one segment.
///  • `printChildren()` accepts a `std::vector<ASTNode*>` and handles the
///    ├──/└── connector selection automatically.

namespace vcc::ast {

class ASTDumper : public ASTVisitorBase {
public:
    /// Construct a dumper that writes to @p out (default: std::cout).
    explicit ASTDumper(std::ostream& out);
    ASTDumper();   ///< Writes to std::cout.

    /// Print the full AST rooted at @p node.
    void dump(ASTNode& node);

    // ── Declarations ──────────────────────────────────────────────────────────
    void visit(ModuleDecl&)      override;
    void visit(ImportDecl&)      override;
    void visit(FunctionDecl&)    override;
    void visit(ParameterDecl&)   override;
    void visit(VarDecl&)         override;
    void visit(StructDecl&)      override;
    void visit(FieldDecl&)       override;
    void visit(EnumDecl&)        override;
    void visit(EnumVariantDecl&) override;
    void visit(TypeAliasDecl&)   override;

    // ── Statements ────────────────────────────────────────────────────────────
    void visit(BlockStmt&)    override;
    void visit(ExprStmt&)     override;
    void visit(ReturnStmt&)   override;
    void visit(IfStmt&)       override;
    void visit(WhileStmt&)    override;
    void visit(VloopStmt&)    override;
    void visit(ForStmt&)      override;
    void visit(BreakStmt&)    override;
    void visit(ContinueStmt&) override;
    void visit(MatchStmt&)    override;
    void visit(ArrayLiteralExpr&) override;

    // ── Expressions ───────────────────────────────────────────────────────────
    void visit(BinaryExpr&)        override;
    void visit(UnaryExpr&)         override;
    void visit(AssignExpr&)        override;
    void visit(CallExpr&)          override;
    void visit(IndexExpr&)         override;
    void visit(MemberExpr&)        override;
    void visit(CastExpr&)          override;
    void visit(IdentExpr&)         override;
    void visit(IntLiteralExpr&)    override;
    void visit(FloatLiteralExpr&)  override;
    void visit(StringLiteralExpr&) override;
    void visit(CharLiteralExpr&)   override;
    void visit(BoolLiteralExpr&)   override;
    void visit(NilLiteralExpr&)    override;

    // ── Types ─────────────────────────────────────────────────────────────────
    void visit(NamedTypeNode&)    override;
    void visit(PointerTypeNode&)  override;
    void visit(SliceTypeNode&)    override;
    void visit(ArrayTypeNode&)    override;
    void visit(FunctionTypeNode&) override;

private:
    // ── Tree connector helpers ─────────────────────────────────────────────────

    /// Print @p children with ├──/└── connectors and proper indentation.
    void printChildren(std::vector<ASTNode*> children);

    /// Overload for vectors of unique_ptr.
    template<typename T>
    void printChildren(const std::vector<std::unique_ptr<T>>& v) {
        std::vector<ASTNode*> ptrs;
        ptrs.reserve(v.size());
        for (const auto& c : v) if (c) ptrs.push_back(c.get());
        printChildren(ptrs);
    }

    /// Print a single node as a child at position @p idx of @p total.
    void printChild(ASTNode& node, bool isLast);

    // ── State ──────────────────────────────────────────────────────────────────
    std::ostream& out_;
    std::string   prefix_;   ///< Accumulated indentation prefix for children
};

} // namespace vcc::ast
