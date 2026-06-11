#include "vcc/ast/ASTDumper.h"

#include "vcc/ast/Declarations.h"
#include "vcc/ast/Expressions.h"
#include "vcc/ast/Statements.h"
#include "vcc/ast/Types.h"

#include <iostream>
#include <string>

namespace vcc::ast {

// ─── Construction ─────────────────────────────────────────────────────────────

ASTDumper::ASTDumper(std::ostream& out) : out_(out) {}
ASTDumper::ASTDumper()                  : out_(std::cout) {}

// ─── Entry point ──────────────────────────────────────────────────────────────

void ASTDumper::dump(ASTNode& node) {
    prefix_.clear();
    node.accept(*this);
}

// ─── Tree connector helpers ───────────────────────────────────────────────────

//  Each child is prefixed with:
//    "├── " if it is NOT the last sibling
//    "└── " if it IS  the last sibling
//  Subsequent children of that child use:
//    "│   " if the sibling above was NOT the last
//    "    " if the sibling above WAS  the last

void ASTDumper::printChild(ASTNode& node, bool isLast) {
    out_ << prefix_ << (isLast ? "└── " : "├── ");
    const std::string saved = prefix_;
    prefix_ += (isLast ? "    " : "│   ");
    node.accept(*this);
    prefix_ = saved;
}

void ASTDumper::printChildren(std::vector<ASTNode*> children) {
    const std::size_t n = children.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (children[i]) printChild(*children[i], i + 1 == n);
    }
}

// ─── Declarations ─────────────────────────────────────────────────────────────

void ASTDumper::visit(ModuleDecl& m) {
    out_ << "Program '" << m.name() << "'\n";
    printChildren(m.decls());
}

void ASTDumper::visit(ImportDecl& imp) {
    out_ << "ImportDecl '" << imp.path() << "'";
    if (imp.alias()) out_ << " as '" << *imp.alias() << "'";
    out_ << '\n';
}

void ASTDumper::visit(FunctionDecl& fn) {
    out_ << "FunctionDecl '" << fn.name() << "'";
    if (fn.isPub()) out_ << " [pub]";
    out_ << '\n';

    std::vector<ASTNode*> children;
    for (auto& p : fn.params())           children.push_back(p.get());
    if (fn.returnType())                  children.push_back(const_cast<TypeNode*>(fn.returnType()));
    if (fn.body())                        children.push_back(const_cast<Stmt*>(fn.body()));
    printChildren(children);
}

void ASTDumper::visit(ParameterDecl& p) {
    out_ << "ParameterDecl '" << p.name() << "'";
    if (p.isMut()) out_ << " [mut]";
    out_ << '\n';
    if (p.type()) {
        std::vector<ASTNode*> ch{const_cast<TypeNode*>(p.type())};
        printChildren(ch);
    }
}

void ASTDumper::visit(VarDecl& v) {
    const char* kw = (v.varKind() == VarKind::Let)   ? "let"
                   : (v.varKind() == VarKind::Var)   ? "var" : "const";
    out_ << "VariableDecl [" << kw << "] '" << v.name() << "'\n";
    std::vector<ASTNode*> children;
    if (v.type())        children.push_back(const_cast<TypeNode*>(v.type()));
    if (v.initializer()) children.push_back(const_cast<Expr*>(v.initializer()));
    printChildren(children);
}

void ASTDumper::visit(StructDecl& s) {
    out_ << "StructDecl '" << s.name() << "'";
    if (s.isPub()) out_ << " [pub]";
    out_ << '\n';
    printChildren(s.fields());
}

void ASTDumper::visit(FieldDecl& f) {
    out_ << "FieldDecl '" << f.name() << "'";
    if (f.isPub()) out_ << " [pub]";
    out_ << '\n';
    if (f.type()) {
        std::vector<ASTNode*> ch{const_cast<TypeNode*>(f.type())};
        printChildren(ch);
    }
}

void ASTDumper::visit(EnumDecl& e) {
    out_ << "EnumDecl '" << e.name() << "'";
    if (e.isPub()) out_ << " [pub]";
    out_ << '\n';
    printChildren(e.variants());
}

void ASTDumper::visit(EnumVariantDecl& v) {
    out_ << "EnumVariantDecl '" << v.name() << "'\n";
    printChildren(v.fields());
}

void ASTDumper::visit(TypeAliasDecl& t) {
    out_ << "TypeAliasDecl '" << t.name() << "'";
    if (t.isPub()) out_ << " [pub]";
    out_ << '\n';
    if (t.aliased()) {
        std::vector<ASTNode*> ch{const_cast<TypeNode*>(t.aliased())};
        printChildren(ch);
    }
}

// ─── Statements ───────────────────────────────────────────────────────────────

void ASTDumper::visit(BlockStmt& b) {
    out_ << "Block\n";
    printChildren(b.stmts());
}

void ASTDumper::visit(ExprStmt& s) {
    out_ << "ExprStmt\n";
    std::vector<ASTNode*> ch{&s.expr()};
    printChildren(ch);
}

void ASTDumper::visit(ReturnStmt& r) {
    out_ << "ReturnStmt\n";
    if (r.value()) {
        std::vector<ASTNode*> ch{const_cast<Expr*>(r.value())};
        printChildren(ch);
    }
}

void ASTDumper::visit(IfStmt& s) {
    out_ << "IfStmt\n";
    std::vector<ASTNode*> children;
    children.push_back(const_cast<Expr*>(&s.condition()));
    children.push_back(const_cast<Stmt*>(&s.thenBranch()));
    if (s.elseBranch()) children.push_back(const_cast<Stmt*>(s.elseBranch()));
    printChildren(children);
}

void ASTDumper::visit(WhileStmt& s) {
    out_ << "WhileStmt\n";
    std::vector<ASTNode*> children{
        const_cast<Expr*>(&s.condition()),
        const_cast<Stmt*>(&s.body())
    };
    printChildren(children);
}

void ASTDumper::visit(VloopStmt& s) {
    out_ << "VloopStmt\n";
    std::vector<ASTNode*> children{
        const_cast<Expr*>(&s.condition()),
        const_cast<Stmt*>(&s.body())
    };
    printChildren(children);
}

void ASTDumper::visit(ForStmt& s) {
    out_ << "ForStmt '" << s.variable() << "'\n";
    std::vector<ASTNode*> children{
        const_cast<Expr*>(&s.iterable()),
        const_cast<Stmt*>(&s.body())
    };
    printChildren(children);
}

void ASTDumper::visit(BreakStmt&)    { out_ << "BreakStmt\n"; }
void ASTDumper::visit(ContinueStmt&) { out_ << "ContinueStmt\n"; }

void ASTDumper::visit(MatchStmt& s) {
    out_ << "MatchStmt\n";
    std::vector<ASTNode*> children{ const_cast<Expr*>(&s.subject()) };
    for (auto& arm : s.arms()) {
        if (arm.pattern) children.push_back(arm.pattern.get());
        children.push_back(arm.body.get());
    }
    printChildren(children);
}

void ASTDumper::visit(ArrayLiteralExpr& e) {
    out_ << "ArrayLiteralExpr[" << e.elements().size() << "]\n";
    std::vector<ASTNode*> children;
    for (auto& elem : e.elements()) children.push_back(elem.get());
    printChildren(children);
}

// ─── Expressions ──────────────────────────────────────────────────────────────

void ASTDumper::visit(BinaryExpr& e) {
    out_ << "BinaryExpr(" << binaryOpName(e.op()) << ")\n";
    std::vector<ASTNode*> children{
        const_cast<Expr*>(&e.lhs()),
        const_cast<Expr*>(&e.rhs())
    };
    printChildren(children);
}

void ASTDumper::visit(UnaryExpr& e) {
    out_ << "UnaryExpr(" << unaryOpName(e.op()) << ")\n";
    std::vector<ASTNode*> ch{const_cast<Expr*>(&e.operand())};
    printChildren(ch);
}

void ASTDumper::visit(AssignExpr& e) {
    const char* opName = "=";
    switch (e.op()) {
        case AssignOp::AddAssign:    opName = "+="; break;
        case AssignOp::SubAssign:    opName = "-="; break;
        case AssignOp::MulAssign:    opName = "*="; break;
        case AssignOp::DivAssign:    opName = "/="; break;
        case AssignOp::ModAssign:    opName = "%="; break;
        case AssignOp::BitAndAssign: opName = "&="; break;
        case AssignOp::BitOrAssign:  opName = "|="; break;
        case AssignOp::BitXorAssign: opName = "^="; break;
        default: break;
    }
    out_ << "AssignExpr(" << opName << ")\n";
    std::vector<ASTNode*> children{
        const_cast<Expr*>(&e.target()),
        const_cast<Expr*>(&e.value())
    };
    printChildren(children);
}

void ASTDumper::visit(CallExpr& e) {
    out_ << "CallExpr\n";
    std::vector<ASTNode*> children;
    children.push_back(const_cast<Expr*>(&e.callee()));
    for (const auto& a : e.args()) children.push_back(const_cast<Expr*>(a.get()));
    printChildren(children);
}

void ASTDumper::visit(IndexExpr& e) {
    out_ << "IndexExpr\n";
    std::vector<ASTNode*> children{
        const_cast<Expr*>(&e.base()),
        const_cast<Expr*>(&e.index())
    };
    printChildren(children);
}

void ASTDumper::visit(MemberExpr& e) {
    out_ << "MemberExpr(." << e.member() << ")\n";
    std::vector<ASTNode*> ch{const_cast<Expr*>(&e.object())};
    printChildren(ch);
}

void ASTDumper::visit(CastExpr& e) {
    out_ << "CastExpr\n";
    std::vector<ASTNode*> children{
        const_cast<Expr*>(&e.expr()),
        const_cast<TypeNode*>(&e.targetType())
    };
    printChildren(children);
}

void ASTDumper::visit(IdentExpr& e) {
    out_ << "IdentifierExpr '" << e.name() << "'\n";
}

void ASTDumper::visit(IntLiteralExpr& e) {
    out_ << "LiteralExpr(int) " << e.value() << '\n';
}

void ASTDumper::visit(FloatLiteralExpr& e) {
    out_ << "LiteralExpr(float) " << e.value() << '\n';
}

void ASTDumper::visit(StringLiteralExpr& e) {
    out_ << "LiteralExpr(string) \"" << e.value() << "\"\n";
}

void ASTDumper::visit(CharLiteralExpr& e) {
    out_ << "LiteralExpr(char) " << static_cast<char>(e.value()) << '\n';
}

void ASTDumper::visit(BoolLiteralExpr& e) {
    out_ << "LiteralExpr(bool) " << (e.value() ? "true" : "false") << '\n';
}

void ASTDumper::visit(NilLiteralExpr&) {
    out_ << "LiteralExpr(nil)\n";
}

// ─── Types ────────────────────────────────────────────────────────────────────

void ASTDumper::visit(NamedTypeNode& t) {
    out_ << "NamedType '" << t.name() << "'\n";
}

void ASTDumper::visit(PointerTypeNode& t) {
    out_ << "PointerType" << (t.isMut() ? " [mut]" : "") << '\n';
    std::vector<ASTNode*> ch{const_cast<TypeNode*>(&t.inner())};
    printChildren(ch);
}

void ASTDumper::visit(SliceTypeNode& t) {
    out_ << "SliceType\n";
    std::vector<ASTNode*> ch{const_cast<TypeNode*>(&t.element())};
    printChildren(ch);
}

void ASTDumper::visit(ArrayTypeNode& t) {
    out_ << "ArrayType\n";
    std::vector<ASTNode*> children{
        const_cast<TypeNode*>(&t.element()),
        const_cast<Expr*>(&t.size())
    };
    printChildren(children);
}

void ASTDumper::visit(FunctionTypeNode& t) {
    out_ << "FunctionType\n";
    std::vector<ASTNode*> children;
    for (const auto& p : t.params()) children.push_back(const_cast<TypeNode*>(p.get()));
    if (t.returnType()) children.push_back(const_cast<TypeNode*>(t.returnType()));
    printChildren(children);
}

} // namespace vcc::ast
