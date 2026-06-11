#pragma once

/// @file ASTVisitor.h
/// Visitor interfaces for traversing the VCC AST.
///
/// Two visitor strategies are provided:
///
///  1. ASTVisitor (pure-virtual interface)
///     Every concrete node kind has an overload.  Implement this when you need
///     guaranteed coverage (e.g. code generation, IR lowering).
///
///  2. ASTVisitorBase (no-op default implementations)
///     Derive from this and override only the nodes you care about
///     (e.g. analysis passes that care only about function calls).
///
/// Future extensions
/// ─────────────────
///  • Pre/post-order hooks (visitPre / visitPost) for recursive walks.
///  • RecursiveASTVisitor that auto-descends into child nodes.
///  • Read-only ConstASTVisitor variant.

namespace vcc::ast {

// Forward-declare all concrete node types so the header stays self-contained.
// Declarations
class ModuleDecl;
class ImportDecl;
class FunctionDecl;
class ParameterDecl;
class StructDecl;
class FieldDecl;
class EnumDecl;
class EnumVariantDecl;
class VarDecl;
class TypeAliasDecl;

// Statements
class BlockStmt;
class ExprStmt;
class ReturnStmt;
class IfStmt;
class WhileStmt;
class VloopStmt;
class ForStmt;
class BreakStmt;
class ContinueStmt;

// Expressions
class BinaryExpr;
class UnaryExpr;
class CallExpr;
class IndexExpr;
class MemberExpr;
class AssignExpr;
class IdentExpr;
class IntLiteralExpr;
class FloatLiteralExpr;
class StringLiteralExpr;
class CharLiteralExpr;
class BoolLiteralExpr;
class NilLiteralExpr;
class CastExpr;

// Types
class NamedTypeNode;
class PointerTypeNode;
class SliceTypeNode;
class ArrayTypeNode;
class FunctionTypeNode;

// ─── Pure-virtual visitor ─────────────────────────────────────────────────────

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // ── Declarations ─────────────────────────────────────────────────────────
    virtual void visit(ModuleDecl&)      = 0;
    virtual void visit(ImportDecl&)      = 0;
    virtual void visit(FunctionDecl&)    = 0;
    virtual void visit(ParameterDecl&)   = 0;
    virtual void visit(StructDecl&)      = 0;
    virtual void visit(FieldDecl&)       = 0;
    virtual void visit(EnumDecl&)        = 0;
    virtual void visit(EnumVariantDecl&) = 0;
    virtual void visit(VarDecl&)         = 0;
    virtual void visit(TypeAliasDecl&)   = 0;

    // ── Statements ───────────────────────────────────────────────────────────
    virtual void visit(BlockStmt&)    = 0;
    virtual void visit(ExprStmt&)     = 0;
    virtual void visit(ReturnStmt&)   = 0;
    virtual void visit(IfStmt&)       = 0;
    virtual void visit(WhileStmt&)    = 0;
    virtual void visit(VloopStmt&)    = 0;
    virtual void visit(ForStmt&)      = 0;
    virtual void visit(BreakStmt&)    = 0;
    virtual void visit(ContinueStmt&) = 0;

    // ── Expressions ──────────────────────────────────────────────────────────
    virtual void visit(BinaryExpr&)       = 0;
    virtual void visit(UnaryExpr&)        = 0;
    virtual void visit(CallExpr&)         = 0;
    virtual void visit(IndexExpr&)        = 0;
    virtual void visit(MemberExpr&)       = 0;
    virtual void visit(AssignExpr&)       = 0;
    virtual void visit(IdentExpr&)        = 0;
    virtual void visit(IntLiteralExpr&)   = 0;
    virtual void visit(FloatLiteralExpr&) = 0;
    virtual void visit(StringLiteralExpr&)= 0;
    virtual void visit(CharLiteralExpr&)  = 0;
    virtual void visit(BoolLiteralExpr&)  = 0;
    virtual void visit(NilLiteralExpr&)   = 0;
    virtual void visit(CastExpr&)         = 0;

    // ── Types ────────────────────────────────────────────────────────────────
    virtual void visit(NamedTypeNode&)    = 0;
    virtual void visit(PointerTypeNode&)  = 0;
    virtual void visit(SliceTypeNode&)    = 0;
    virtual void visit(ArrayTypeNode&)    = 0;
    virtual void visit(FunctionTypeNode&) = 0;
};

// ─── No-op base for selective overrides ──────────────────────────────────────

class ASTVisitorBase : public ASTVisitor {
public:
    // All overrides are no-ops. Derive and override only what you need.
    void visit(ModuleDecl&)       override {}
    void visit(ImportDecl&)       override {}
    void visit(FunctionDecl&)     override {}
    void visit(ParameterDecl&)    override {}
    void visit(StructDecl&)       override {}
    void visit(FieldDecl&)        override {}
    void visit(EnumDecl&)         override {}
    void visit(EnumVariantDecl&)  override {}
    void visit(VarDecl&)          override {}
    void visit(TypeAliasDecl&)    override {}

    void visit(BlockStmt&)        override {}
    void visit(ExprStmt&)         override {}
    void visit(ReturnStmt&)       override {}
    void visit(IfStmt&)           override {}
    void visit(WhileStmt&)        override {}
    void visit(ForStmt&)          override {}
    void visit(BreakStmt&)        override {}
    void visit(ContinueStmt&)     override {}

    void visit(BinaryExpr&)        override {}
    void visit(UnaryExpr&)         override {}
    void visit(CallExpr&)          override {}
    void visit(IndexExpr&)         override {}
    void visit(MemberExpr&)        override {}
    void visit(AssignExpr&)        override {}
    void visit(IdentExpr&)         override {}
    void visit(IntLiteralExpr&)    override {}
    void visit(FloatLiteralExpr&)  override {}
    void visit(StringLiteralExpr&) override {}
    void visit(CharLiteralExpr&)   override {}
    void visit(BoolLiteralExpr&)   override {}
    void visit(NilLiteralExpr&)    override {}
    void visit(CastExpr&)          override {}

    void visit(NamedTypeNode&)     override {}
    void visit(PointerTypeNode&)   override {}
    void visit(SliceTypeNode&)     override {}
    void visit(ArrayTypeNode&)     override {}
    void visit(FunctionTypeNode&)  override {}
};

} // namespace vcc::ast
