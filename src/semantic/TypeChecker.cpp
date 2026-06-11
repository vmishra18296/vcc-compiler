#include "vcc/semantic/TypeChecker.h"

#include "vcc/ast/Statements.h"
#include "vcc/ast/Types.h"

namespace vcc::semantic {

using namespace vcc::ast;
using namespace vcc::common;

// ─── Internal helpers ─────────────────────────────────────────────────────────

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

TypeChecker::TypeChecker(CompilerContext& ctx, SymbolTable& symbols)
    : ctx_(ctx), symbols_(symbols) {}

void TypeChecker::check(ModuleDecl& module) {
    module.accept(*this);
}

// ─── Private helpers ──────────────────────────────────────────────────────────

const std::string& TypeChecker::exprType() const noexcept {
    return lastExprType_;
}

void TypeChecker::setExprType(std::string t) {
    lastExprType_ = std::move(t);
}

bool TypeChecker::typesCompatible(const std::string& a,
                                   const std::string& b) const noexcept {
    if (a == b)                           return true;
    if (a == "<any>" || b == "<any>")     return true;  // unresolved
    if (a.empty()    || b.empty())        return true;  // unannotated
    // Numeric aliases: treat i8/i16/i32/i64/isize as compatible with each other,
    // same for f32/f64, to avoid noise while the type system is evolving.
    auto isInt   = [](const std::string& s){ return s=="i8"||s=="i16"||s=="i32"||s=="i64"||s=="isize"||s=="u8"||s=="u16"||s=="u32"||s=="u64"||s=="usize"; };
    auto isFloat = [](const std::string& s){ return s=="f32"||s=="f64"; };
    if (isInt(a)   && isInt(b))   return true;
    if (isFloat(a) && isFloat(b)) return true;
    return false;
}

// ─── Module / declarations ────────────────────────────────────────────────────

void TypeChecker::visit(ModuleDecl& m) {
    for (auto& d : m.decls()) d->accept(*this);
}

void TypeChecker::visit(FunctionDecl& fn) {
    // Determine whether this function declares a return type.
    const std::string retType = typeNodeToString(fn.returnType());
    const std::string prevRetType = currentReturnType_;
    const bool        prevHas     = hasReturn_;
    const bool        prevReq     = requiresReturn_;

    currentReturnType_ = retType;
    hasReturn_         = false;
    requiresReturn_    = !retType.empty();

    symbols_.enterScope("fn:" + fn.name());
    for (auto& p : fn.params()) p->accept(*this);
    if (fn.body()) const_cast<Stmt*>(fn.body())->accept(*this);
    symbols_.leaveScope();

    // Check 5: function with declared return type must have at least one return.
    if (requiresReturn_ && !hasReturn_) {
        ctx_.diagnostics().error(fn.range().begin,
            "function '" + fn.name() + "' must return a value of type '" +
            retType + "' but control reaches end without a return statement");
    }

    // Restore saved state (handles nested fn once closures are added).
    currentReturnType_ = prevRetType;
    hasReturn_         = prevHas;
    requiresReturn_    = prevReq;
}

// Define the parameter in TypeChecker's own scope so body expressions can
// look up its type via symbols_.lookup().
void TypeChecker::visit(ParameterDecl& p) {
    Symbol sym;
    sym.name     = p.name();
    sym.kind     = SymbolKind::Parameter;
    sym.isMut    = p.isMut();
    sym.typeName = typeNodeToString(p.type());
    sym.declLoc  = p.range().begin;
    symbols_.define(sym);  // duplicate already reported by SemanticAnalyzer
}

void TypeChecker::visit(StructDecl&) {
    // Future: validate field types, recursive layout, etc.
}

// Check 3: type compatibility between annotation and initializer.
void TypeChecker::visit(VarDecl& v) {
    std::string declaredType = typeNodeToString(v.type());

    if (v.initializer()) {
        const_cast<Expr*>(v.initializer())->accept(*this);
        const std::string inferredType = exprType();

        if (!declaredType.empty() && !inferredType.empty()) {
            if (!typesCompatible(declaredType, inferredType)) {
                ctx_.diagnostics().error(v.range().begin,
                    "type mismatch in initializer for '" + v.name() +
                    "': declared '" + declaredType +
                    "', got '" + inferredType + "'");
            }
        }
        // Use inferred type if no annotation.
        if (declaredType.empty()) declaredType = inferredType;
    }

    // Define var in TypeChecker's scope so subsequent expressions can
    // look up its type.
    Symbol sym;
    sym.name     = v.name();
    sym.kind     = (v.varKind() == VarKind::Const) ? SymbolKind::Constant
                                                    : SymbolKind::Variable;
    sym.isMut    = (v.varKind() == VarKind::Var);
    sym.typeName = declaredType;
    sym.declLoc  = v.range().begin;
    symbols_.define(sym);  // duplicate already reported by SemanticAnalyzer
}

// ─── Statements ───────────────────────────────────────────────────────────────

void TypeChecker::visit(BlockStmt& b) {
    symbols_.enterScope("block");
    for (auto& s : b.stmts()) s->accept(*this);
    symbols_.leaveScope();
}

// Check 5: validate return type matches function's declared return type.
void TypeChecker::visit(ReturnStmt& r) {
    hasReturn_ = true;  // at least one return exists in this function

    if (r.value()) {
        const_cast<Expr*>(r.value())->accept(*this);
        const std::string retType = exprType();

        if (!currentReturnType_.empty() && !retType.empty()) {
            if (!typesCompatible(retType, currentReturnType_)) {
                ctx_.diagnostics().error(r.range().begin,
                    "return type mismatch: expected '" + currentReturnType_ +
                    "', got '" + retType + "'");
            }
        }
    } else if (!currentReturnType_.empty()) {
        ctx_.diagnostics().error(r.range().begin,
            "missing return value: function returns '" +
            currentReturnType_ + "'");
    }
}

void TypeChecker::visit(IfStmt& s) {
    const_cast<Expr&>(s.condition()).accept(*this);
    if (exprType() != "bool" && exprType() != "<any>" && !exprType().empty()) {
        ctx_.diagnostics().warning(s.condition().range().begin,
            "if condition should be 'bool', got '" + exprType() + "'");
    }
    const_cast<Stmt&>(s.thenBranch()).accept(*this);
    if (s.elseBranch()) const_cast<Stmt*>(s.elseBranch())->accept(*this);
}

void TypeChecker::visit(WhileStmt& s) {
    const_cast<Expr&>(s.condition()).accept(*this);
    if (exprType() != "bool" && exprType() != "<any>" && !exprType().empty()) {
        ctx_.diagnostics().warning(s.condition().range().begin,
            "while condition should be 'bool', got '" + exprType() + "'");
    }
    const_cast<Stmt&>(s.body()).accept(*this);
}

void TypeChecker::visit(VloopStmt& s) {
    const_cast<Expr&>(s.condition()).accept(*this);
    if (exprType() != "bool" && exprType() != "<any>" && !exprType().empty()) {
        ctx_.diagnostics().warning(s.condition().range().begin,
            "vloop condition should be 'bool', got '" + exprType() + "'");
    }
    const_cast<Stmt&>(s.body()).accept(*this);
}

void TypeChecker::visit(MatchStmt& s) {
    const_cast<Expr&>(s.subject()).accept(*this);
    for (auto& arm : s.arms()) {
        if (arm.pattern) const_cast<Expr&>(*arm.pattern).accept(*this);
        const_cast<Stmt&>(*arm.body).accept(*this);
    }
}

void TypeChecker::visit(ArrayLiteralExpr& e) {
    for (auto& elem : e.elements())
        const_cast<Expr&>(*elem).accept(*this);
    setExprType("array");
}

void TypeChecker::visit(ForStmt& s) {
    symbols_.enterScope("for");
    // Define loop variable in TC's scope (type inferred from iterable — <any> for now).
    Symbol loopVar;
    loopVar.name     = s.variable();
    loopVar.kind     = SymbolKind::Variable;
    loopVar.typeName = "<any>";
    symbols_.define(loopVar);

    const_cast<Expr&>(s.iterable()).accept(*this);
    const_cast<Stmt&>(s.body()).accept(*this);
    symbols_.leaveScope();
}

void TypeChecker::visit(ExprStmt& s) {
    s.expr().accept(*this);
}

// ─── Expressions ─────────────────────────────────────────────────────────────

void TypeChecker::visit(IntLiteralExpr&)    { setExprType("i64"); }
void TypeChecker::visit(FloatLiteralExpr&)  { setExprType("f64"); }
void TypeChecker::visit(BoolLiteralExpr&)   { setExprType("bool"); }
void TypeChecker::visit(StringLiteralExpr&) { setExprType("str"); }

// Check 1 (already caught by SemanticAnalyzer; here we just look up the type).
void TypeChecker::visit(IdentExpr& e) {
    const Symbol* sym = symbols_.lookup(e.name());
    if (!sym) {
        setExprType("<any>");
        return;
    }
    setExprType(sym->typeName.empty() ? "<any>" : sym->typeName);
}

// Check 3: binary expression operand types must be compatible.
void TypeChecker::visit(BinaryExpr& e) {
    e.lhs().accept(*this);
    const std::string lhsType = exprType();
    e.rhs().accept(*this);
    const std::string rhsType = exprType();

    if (!typesCompatible(lhsType, rhsType)) {
        ctx_.diagnostics().error(e.range().begin,
            "type mismatch in binary expression: '" + lhsType +
            "' vs '" + rhsType + "'");
    }
    setExprType(lhsType);
}

void TypeChecker::visit(UnaryExpr& e) {
    e.operand().accept(*this);
    // Future: validate unary op against operand type (e.g. !bool, -numeric).
}

// Check 4: function call — look up return type; validate argument types.
void TypeChecker::visit(CallExpr& e) {
    const_cast<Expr&>(e.callee()).accept(*this);

    // Collect argument types.
    for (auto& arg : e.args()) {
        const_cast<Expr&>(*arg).accept(*this);
        // Future: compare arg type against corresponding parameter type.
    }

    // Derive result type from the function symbol.
    std::string resultType = "<any>";
    if (const auto* ident = dynamic_cast<const IdentExpr*>(&e.callee())) {
        if (const Symbol* sym = symbols_.lookup(ident->name())) {
            resultType = sym->returnType.empty() ? "<any>" : sym->returnType;
        }
    }
    setExprType(resultType);
}

// Check 3: assignment — target and value types must be compatible.
void TypeChecker::visit(AssignExpr& e) {
    const_cast<Expr&>(e.target()).accept(*this);
    const std::string targetType = exprType();
    const_cast<Expr&>(e.value()).accept(*this);
    const std::string valueType = exprType();

    if (!typesCompatible(targetType, valueType)) {
        ctx_.diagnostics().error(e.range().begin,
            "type mismatch in assignment: cannot assign '" +
            valueType + "' to '" + targetType + "'");
    }
    setExprType(targetType);
}

} // namespace vcc::semantic
