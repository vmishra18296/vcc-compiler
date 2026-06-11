#include "vcc/semantic/SemanticAnalyzer.h"

#include "vcc/ast/Statements.h"
#include "vcc/ast/Types.h"

#include <string>

namespace vcc::semantic {

using namespace vcc::ast;
using namespace vcc::common;

// ─── Internal helpers ─────────────────────────────────────────────────────────

/// Recursively convert a TypeNode to its canonical string name.
static std::string typeNodeToString(const TypeNode* t) {
    if (!t) return "";
    if (auto* named = dynamic_cast<const NamedTypeNode*>(t))
        return named->name();
    if (auto* ptr = dynamic_cast<const PointerTypeNode*>(t))
        return (ptr->isMut() ? "*mut " : "*") + typeNodeToString(&ptr->inner());
    if (auto* arr = dynamic_cast<const ArrayTypeNode*>(t))
        return "[" + typeNodeToString(&arr->element()) + "]";
    return "<type>";
}

// ─── Constructor ──────────────────────────────────────────────────────────────

SemanticAnalyzer::SemanticAnalyzer(CompilerContext& ctx)
    : ctx_(ctx) {}

// ─── analyze ─────────────────────────────────────────────────────────────────

bool SemanticAnalyzer::analyze(ModuleDecl& module) {
    const uint32_t errsBefore = ctx_.diagnostics().errorCount();

    // ── Built-in functions ────────────────────────────────────────────────────
    // Pre-declare built-ins so user code can call them without import.
    auto declareBuiltin = [&](std::string name) {
        Symbol s;
        s.name       = std::move(name);
        s.kind       = SymbolKind::Function;
        s.returnType = "<any>";
        s.typeName   = "<any>";
        symbols_.define(s);
    };
    declareBuiltin("print");
    declareBuiltin("println");
    declareBuiltin("eprint");
    declareBuiltin("eprintln");
    declareBuiltin("len");
    declareBuiltin("sizeof");
    declareBuiltin("exit");    // exit(code=0)  — terminate program
    declareBuiltin("escape");  // escape(code=1) — terminate with error
    declareBuiltin("wait");    // wait(ms)       — sleep milliseconds

    // ── Pass 1: hoist all top-level declarations ──────────────────────────────
    // Pre-declare functions and types so forward references work.
    for (const auto& decl : module.decls()) {
        if (auto* fn = dynamic_cast<FunctionDecl*>(decl.get())) {
            // Build a Symbol carrying FunctionSymbol metadata (base-field
            // storage avoids object slicing in the unordered_map).
            Symbol fsym;
            fsym.name       = fn->name();
            fsym.kind       = SymbolKind::Function;
            fsym.isPub      = fn->isPub();
            fsym.returnType = typeNodeToString(fn->returnType());
            fsym.typeName   = fsym.returnType;
            fsym.declLoc    = fn->range().begin;
            for (const auto& p : fn->params())
                fsym.paramTypes.push_back(typeNodeToString(p->type()));

            if (!symbols_.define(fsym)) {
                const Symbol* prev = symbols_.lookupCurrent(fn->name());
                ctx_.diagnostics().error(fn->range().begin,
                    "redefinition of function '" + fn->name() + "'");
                if (prev && prev->declLoc.isValid())
                    ctx_.diagnostics().note(prev->declLoc,
                        "'" + fn->name() + "' first defined here");
            }

        } else if (auto* st = dynamic_cast<StructDecl*>(decl.get())) {
            Symbol sym;
            sym.name    = st->name();
            sym.kind    = SymbolKind::Type;
            sym.isPub   = st->isPub();
            sym.declLoc = st->range().begin;
            if (!symbols_.define(sym)) {
                ctx_.diagnostics().error(st->range().begin,
                    "redefinition of type '" + st->name() + "'");
            }

        } else if (auto* en = dynamic_cast<EnumDecl*>(decl.get())) {
            Symbol sym;
            sym.name    = en->name();
            sym.kind    = SymbolKind::Type;
            sym.isPub   = en->isPub();
            sym.declLoc = en->range().begin;
            if (!symbols_.define(sym)) {
                ctx_.diagnostics().error(en->range().begin,
                    "redefinition of type '" + en->name() + "'");
            }
        }
    }

    // ── Pass 2: name resolution + scope / mutation checks ────────────────────
    module.accept(*this);

    // ── Pass 3: type checking ─────────────────────────────────────────────────
    TypeChecker tc(ctx_, symbols_);
    tc.check(module);

    return ctx_.diagnostics().errorCount() == errsBefore;
}

// ─── Module ───────────────────────────────────────────────────────────────────

void SemanticAnalyzer::visit(ModuleDecl& m) {
    for (auto& d : m.decls()) d->accept(*this);
}

// ─── Import ───────────────────────────────────────────────────────────────────

void SemanticAnalyzer::visit(ImportDecl& imp) {
    const std::string name = imp.alias().value_or(imp.path());
    Symbol sym;
    sym.name    = name;
    sym.kind    = SymbolKind::Module;
    sym.declLoc = imp.range().begin;
    if (!symbols_.define(sym)) {
        ctx_.diagnostics().warning(imp.range().begin,
            "duplicate import '" + imp.path() + "'");
    }
}

// ─── Function ─────────────────────────────────────────────────────────────────

void SemanticAnalyzer::visit(FunctionDecl& fn) {
    const std::string saved = currentFunction_;
    currentFunction_ = fn.name();

    symbols_.enterScope("fn:" + fn.name());
    for (auto& p : fn.params()) p->accept(*this);
    if (fn.body()) const_cast<Stmt*>(fn.body())->accept(*this);
    symbols_.leaveScope();

    currentFunction_ = saved;
}

// Check 2: duplicate parameter names; stores typeName for the TypeChecker.
void SemanticAnalyzer::visit(ParameterDecl& p) {
    Symbol sym;
    sym.name     = p.name();
    sym.kind     = SymbolKind::Parameter;
    sym.isMut    = p.isMut();
    sym.typeName = typeNodeToString(p.type());
    sym.declLoc  = p.range().begin;

    if (!symbols_.define(sym)) {
        const Symbol* prev = symbols_.lookupCurrent(p.name());
        ctx_.diagnostics().error(p.range().begin,
            "duplicate parameter '" + p.name() + "'");
        if (prev && prev->declLoc.isValid())
            ctx_.diagnostics().note(prev->declLoc,
                "'" + p.name() + "' previously declared here");
    }
}

// ─── Struct / Enum / Type alias ───────────────────────────────────────────────

void SemanticAnalyzer::visit(StructDecl&)    { /* fields validated by TypeChecker */ }
void SemanticAnalyzer::visit(EnumDecl&)      { /* variants validated by TypeChecker */ }
void SemanticAnalyzer::visit(TypeAliasDecl&) {}

// ─── Variable declarations ───────────────────────────────────────────────────

// Check 1 (declared before use) + Check 2 (duplicate declarations).
void SemanticAnalyzer::visit(VarDecl& v) {
    // Evaluate initializer first — variable not yet in scope.
    if (v.initializer()) const_cast<Expr*>(v.initializer())->accept(*this);

    bool isMut = (v.varKind() == VarKind::Var);
    SymbolKind kind = (v.varKind() == VarKind::Const) ? SymbolKind::Constant
                                                       : SymbolKind::Variable;
    Symbol sym;
    sym.name     = v.name();
    sym.kind     = kind;
    sym.isMut    = isMut;
    sym.typeName = typeNodeToString(v.type());
    sym.declLoc  = v.range().begin;

    if (!symbols_.define(sym)) {
        const Symbol* prev = symbols_.lookupCurrent(v.name());
        ctx_.diagnostics().error(v.range().begin,
            "redefinition of '" + v.name() + "'");
        if (prev && prev->declLoc.isValid())
            ctx_.diagnostics().note(prev->declLoc,
                "'" + v.name() + "' previously declared here");
    }
}

// ─── Statements ───────────────────────────────────────────────────────────────

// Check 6: each block creates a nested scope.
void SemanticAnalyzer::visit(BlockStmt& b) {
    symbols_.enterScope("block");
    for (auto& s : b.stmts()) s->accept(*this);
    symbols_.leaveScope();
}

void SemanticAnalyzer::visit(IfStmt& s) {
    const_cast<Expr&>(s.condition()).accept(*this);
    const_cast<Stmt&>(s.thenBranch()).accept(*this);
    if (s.elseBranch()) const_cast<Stmt*>(s.elseBranch())->accept(*this);
}

void SemanticAnalyzer::visit(WhileStmt& s) {
    const_cast<Expr&>(s.condition()).accept(*this);
    const_cast<Stmt&>(s.body()).accept(*this);
}

void SemanticAnalyzer::visit(VloopStmt& s) {
    const_cast<Expr&>(s.condition()).accept(*this);
    const_cast<Stmt&>(s.body()).accept(*this);
}

void SemanticAnalyzer::visit(MatchStmt& s) {
    const_cast<Expr&>(s.subject()).accept(*this);
    for (auto& arm : s.arms()) {
        if (arm.pattern) const_cast<Expr&>(*arm.pattern).accept(*this);
        const_cast<Stmt&>(*arm.body).accept(*this);
    }
}

void SemanticAnalyzer::visit(ArrayLiteralExpr& e) {
    for (auto& elem : e.elements())
        const_cast<Expr&>(*elem).accept(*this);
}

void SemanticAnalyzer::visit(ForStmt& s) {
    // Loop variable is scoped to the for body.
    symbols_.enterScope("for");
    Symbol loopVar;
    loopVar.name    = s.variable();
    loopVar.kind    = SymbolKind::Variable;
    loopVar.isMut   = false;
    loopVar.declLoc = s.range().begin;
    symbols_.define(loopVar);

    const_cast<Expr&>(s.iterable()).accept(*this);
    const_cast<Stmt&>(s.body()).accept(*this);
    symbols_.leaveScope();
}

// Check 5: 'return' only valid inside a function (type match handled by TypeChecker).
void SemanticAnalyzer::visit(ReturnStmt& s) {
    if (currentFunction_.empty()) {
        ctx_.diagnostics().error(s.range().begin,
            "'return' outside of a function");
        return;
    }
    if (s.value()) const_cast<Expr*>(s.value())->accept(*this);
}

void SemanticAnalyzer::visit(ExprStmt& s) {
    s.expr().accept(*this);
}

// ─── Expressions ─────────────────────────────────────────────────────────────

// Check 1: variable declared before use.
void SemanticAnalyzer::visit(IdentExpr& e) {
    if (!symbols_.lookup(e.name())) {
        ctx_.diagnostics().error(e.range().begin,
            "use of undeclared identifier '" + e.name() + "'");
    }
}

// Check 4: function existence + argument-count validation.
void SemanticAnalyzer::visit(CallExpr& e) {
    if (const auto* ident = dynamic_cast<const IdentExpr*>(&e.callee())) {
        const Symbol* sym = symbols_.lookup(ident->name());
        if (!sym) {
            ctx_.diagnostics().error(e.callee().range().begin,
                "call to undeclared function '" + ident->name() + "'");
        } else if (sym->kind != SymbolKind::Function) {
            ctx_.diagnostics().error(e.callee().range().begin,
                "'" + ident->name() + "' is not a function");
        } else {
            // Skip arg-count check for built-ins (variadic / typeName == "<any>")
            // and for calls to functions whose signature isn't fully known yet.
            if (sym->typeName != "<any>" && !sym->paramTypes.empty() &&
                sym->paramTypes.size() != e.args().size()) {
                ctx_.diagnostics().error(e.range().begin,
                    "wrong number of arguments to '" + ident->name() +
                    "': expected " + std::to_string(sym->paramTypes.size()) +
                    ", got " + std::to_string(e.args().size()));
            }
        }
    } else {
        const_cast<Expr&>(e.callee()).accept(*this);
    }
    for (auto& arg : e.args()) const_cast<Expr&>(*arg).accept(*this);
}

void SemanticAnalyzer::visit(BinaryExpr& e) {
    e.lhs().accept(*this);
    e.rhs().accept(*this);
}

void SemanticAnalyzer::visit(UnaryExpr& e) {
    e.operand().accept(*this);
}

// Check 6: scope mutation guard — const and immutable let cannot be reassigned.
void SemanticAnalyzer::visit(AssignExpr& e) {
    if (const auto* ident = dynamic_cast<const IdentExpr*>(&e.target())) {
        const Symbol* sym = symbols_.lookup(ident->name());
        if (!sym) {
            // V language v0.1: bare assignment to undeclared name
            // implicitly declares a mutable variable (type inference).
            Symbol implied;
            implied.name    = ident->name();
            implied.kind    = SymbolKind::Variable;
            implied.isMut   = true;
            implied.declLoc = e.range().begin;
            implied.typeName = "<inferred>";
            symbols_.define(implied);
        } else {
            if (sym->kind == SymbolKind::Constant) {
                ctx_.diagnostics().error(e.range().begin,
                    "cannot assign to '" + ident->name() +
                    "': declared as 'const'");
                if (sym->declLoc.isValid())
                    ctx_.diagnostics().note(sym->declLoc,
                        "'" + ident->name() + "' declared const here");

            } else if (sym->kind == SymbolKind::Variable && !sym->isMut) {
                ctx_.diagnostics().error(e.range().begin,
                    "cannot assign to '" + ident->name() +
                    "': declared as immutable 'let'; use 'var' for a mutable binding");
                if (sym->declLoc.isValid())
                    ctx_.diagnostics().note(sym->declLoc,
                        "'" + ident->name() + "' declared here");

            } else if (sym->kind == SymbolKind::Function) {
                ctx_.diagnostics().error(e.range().begin,
                    "cannot assign to function '" + ident->name() + "'");
            }
        }
    }
    const_cast<Expr&>(e.target()).accept(*this);
    const_cast<Expr&>(e.value()).accept(*this);
}

void SemanticAnalyzer::visit(MemberExpr& e) {
    const_cast<Expr&>(e.object()).accept(*this);
    // Future: verify field exists in the resolved struct type.
}

} // namespace vcc::semantic
