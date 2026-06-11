#include <gtest/gtest.h>

#include "vcc/ast/Declarations.h"
#include "vcc/ast/Expressions.h"
#include "vcc/ast/Statements.h"
#include "vcc/common/CompilerContext.h"
#include "vcc/lexer/Lexer.h"
#include "vcc/parser/Parser.h"

using namespace vcc::common;
using namespace vcc::lexer;
using namespace vcc::parser;
using namespace vcc::ast;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static std::unique_ptr<ModuleDecl> parse(const std::string& src,
                                          CompilerContext*    outCtx = nullptr) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", src);
    Lexer lex(ctx, id);
    Parser parser(ctx, lex.tokenize());
    auto mod = parser.parse();
    if (outCtx) *outCtx = std::move(ctx);  // expose context for error checking
    return mod;
}

// ─── Module ───────────────────────────────────────────────────────────────────

TEST(ParserTest, EmptyModule) {
    auto mod = parse("module test\n");
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->name(), "test");
    EXPECT_TRUE(mod->decls().empty());
}

TEST(ParserTest, AnonModule) {
    auto mod = parse("");
    ASSERT_NE(mod, nullptr);
}

// ─── Functions ────────────────────────────────────────────────────────────────

TEST(ParserTest, EmptyFunction) {
    auto mod = parse("fn foo() {}");
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->decls().size(), 1u);
    auto* fn = dynamic_cast<FunctionDecl*>(mod->decls()[0].get());
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->name(), "foo");
    EXPECT_TRUE(fn->params().empty());
    EXPECT_EQ(fn->returnType(), nullptr);
}

TEST(ParserTest, FunctionWithReturnType) {
    auto mod = parse("fn add(a: i32, b: i32) -> i32 { return a }");
    ASSERT_NE(mod, nullptr);
    auto* fn = dynamic_cast<FunctionDecl*>(mod->decls()[0].get());
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->name(), "add");
    EXPECT_EQ(fn->params().size(), 2u);
    ASSERT_NE(fn->returnType(), nullptr);
    auto* retType = dynamic_cast<const NamedTypeNode*>(fn->returnType());
    ASSERT_NE(retType, nullptr);
    EXPECT_EQ(retType->name(), "i32");
}

TEST(ParserTest, PubFunction) {
    auto mod = parse("pub fn greet() {}");
    ASSERT_NE(mod, nullptr);
    auto* fn = dynamic_cast<FunctionDecl*>(mod->decls()[0].get());
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->isPub());
}

// ─── Variable declarations ────────────────────────────────────────────────────

TEST(ParserTest, LetBinding) {
    auto mod = parse("fn f() { let x = 42 }");
    ASSERT_NE(mod, nullptr);
    auto* fn = dynamic_cast<FunctionDecl*>(mod->decls()[0].get());
    ASSERT_NE(fn, nullptr);
    auto* body = dynamic_cast<const BlockStmt*>(fn->body());
    ASSERT_NE(body, nullptr);
    ASSERT_EQ(body->stmts().size(), 1u);
}

TEST(ParserTest, LetBindingWithType) {
    auto mod = parse("fn f() { let x: i64 = 0 }");
    ASSERT_NE(mod, nullptr);
}

// ─── Expressions ─────────────────────────────────────────────────────────────

TEST(ParserTest, BinaryExpression) {
    auto mod = parse("fn f() { let r = 1 + 2 * 3 }");
    ASSERT_NE(mod, nullptr);
}

TEST(ParserTest, FunctionCall) {
    auto mod = parse("fn f() { foo(1, 2) }");
    ASSERT_NE(mod, nullptr);
}

TEST(ParserTest, MemberAccess) {
    auto mod = parse("fn f() { obj.field }");
    ASSERT_NE(mod, nullptr);
}

// ─── Statements ───────────────────────────────────────────────────────────────

TEST(ParserTest, IfStatement) {
    auto mod = parse("fn f() { if true { } }");
    ASSERT_NE(mod, nullptr);
}

TEST(ParserTest, IfElseStatement) {
    auto mod = parse("fn f() { if x { } else { } }");
    ASSERT_NE(mod, nullptr);
}

TEST(ParserTest, WhileStatement) {
    auto mod = parse("fn f() { while true { } }");
    ASSERT_NE(mod, nullptr);
}

TEST(ParserTest, ForInStatement) {
    auto mod = parse("fn f() { for i in items { } }");
    ASSERT_NE(mod, nullptr);
}

TEST(ParserTest, ReturnWithValue) {
    auto mod = parse("fn f() -> i32 { return 42 }");
    ASSERT_NE(mod, nullptr);
}

// ─── Struct ───────────────────────────────────────────────────────────────────

TEST(ParserTest, StructDecl) {
    auto mod = parse("struct Point { x: f64, y: f64 }");
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->decls().size(), 1u);
    auto* st = dynamic_cast<StructDecl*>(mod->decls()[0].get());
    ASSERT_NE(st, nullptr);
    EXPECT_EQ(st->name(), "Point");
    EXPECT_EQ(st->fields().size(), 2u);
}

// ─── Import ───────────────────────────────────────────────────────────────────

TEST(ParserTest, ImportDecl) {
    auto mod = parse("import std.io");
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->decls().size(), 1u);
    auto* imp = dynamic_cast<ImportDecl*>(mod->decls()[0].get());
    ASSERT_NE(imp, nullptr);
    EXPECT_EQ(imp->path(), "std.io");
    EXPECT_FALSE(imp->alias().has_value());
}

TEST(ParserTest, ImportWithAlias) {
    auto mod = parse("import math as m");
    ASSERT_NE(mod, nullptr);
    auto* imp = dynamic_cast<ImportDecl*>(mod->decls()[0].get());
    ASSERT_NE(imp, nullptr);
    EXPECT_EQ(imp->alias().value(), "m");
}

// ─── Type annotations ────────────────────────────────────────────────────────

TEST(ParserTest, PointerType) {
    auto mod = parse("fn f(p: *i32) {}");
    ASSERT_NE(mod, nullptr);
}

TEST(ParserTest, MutPointerType) {
    auto mod = parse("fn f(p: *mut i32) {}");
    ASSERT_NE(mod, nullptr);
}

TEST(ParserTest, SliceType) {
    auto mod = parse("fn f(s: [u8]) {}");
    ASSERT_NE(mod, nullptr);
}

// ─── Error recovery ──────────────────────────────────────────────────────────

TEST(ParserTest, RecoveryAfterMissingBrace) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", "fn f( {}");
    Lexer lex(ctx, id);
    Parser parser(ctx, lex.tokenize());
    auto mod = parser.parse();
    // Parser should emit an error but not crash
    EXPECT_GT(ctx.diagnostics().errorCount(), 0u);
}

// ═══════════════════════════════════════════════════════════════════════════
// Pratt expression parser tests
// ─────────────────────────────
// These tests call parseExpression() directly without going through a full
// module parse, verifying the Pratt algorithm's precedence and associativity.
// ═══════════════════════════════════════════════════════════════════════════

/// Lex @p src, construct a parser, and return the expression parsed at pos 0.
static std::unique_ptr<Expr> parseExpr(const std::string& src) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", src);
    Lexer lex(ctx, id);
    Parser parser(ctx, lex.tokenize());
    return parser.parseExpression();
}

// ─── Helper casts ─────────────────────────────────────────────────────────────

static const BinaryExpr* asBinary(const Expr* e) {
    return dynamic_cast<const BinaryExpr*>(e);
}
static const UnaryExpr* asUnary(const Expr* e) {
    return dynamic_cast<const UnaryExpr*>(e);
}
static const IntLiteralExpr* asInt(const Expr* e) {
    return dynamic_cast<const IntLiteralExpr*>(e);
}
static const IdentExpr* asIdent(const Expr* e) {
    return dynamic_cast<const IdentExpr*>(e);
}
static const LiteralExpr* asLiteral(const Expr* e) {
    return dynamic_cast<const LiteralExpr*>(e);
}

// ─── Requirement-spec examples ────────────────────────────────────────────────

// 1 + 2 * 3  →  BinaryExpr(Add, 1, BinaryExpr(Mul, 2, 3))
// Multiplication must bind tighter than addition.
TEST(PrattTest, MulBindsTighterThanAdd) {
    auto expr = parseExpr("1 + 2 * 3");
    auto* add = asBinary(expr.get());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op(), BinaryOp::Add);

    // LHS must be the literal 1
    auto* lhs = asInt(&add->lhs());
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ(lhs->value(), 1);

    // RHS must be (2 * 3)
    auto* mul = asBinary(&add->rhs());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op(), BinaryOp::Mul);
    EXPECT_EQ(asInt(&mul->lhs())->value(), 2);
    EXPECT_EQ(asInt(&mul->rhs())->value(), 3);
}

// (a+b)*c  →  BinaryExpr(Mul, BinaryExpr(Add, a, b), c)
// Parentheses override default precedence.
TEST(PrattTest, ParenthesesOverridePrecedence) {
    auto expr = parseExpr("(a+b)*c");
    auto* mul = asBinary(expr.get());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op(), BinaryOp::Mul);

    // LHS must be (a+b)
    auto* add = asBinary(&mul->lhs());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op(), BinaryOp::Add);
    EXPECT_EQ(asIdent(&add->lhs())->name(), "a");
    EXPECT_EQ(asIdent(&add->rhs())->name(), "b");

    // RHS must be c
    EXPECT_EQ(asIdent(&mul->rhs())->name(), "c");
}

// x==y  →  BinaryExpr(Eq, x, y)
TEST(PrattTest, EqualityExpression) {
    auto expr = parseExpr("x==y");
    auto* eq = asBinary(expr.get());
    ASSERT_NE(eq, nullptr);
    EXPECT_EQ(eq->op(), BinaryOp::Eq);
    EXPECT_EQ(asIdent(&eq->lhs())->name(), "x");
    EXPECT_EQ(asIdent(&eq->rhs())->name(), "y");
}

// x&&y  →  BinaryExpr(And, x, y)
TEST(PrattTest, LogicalAndExpression) {
    auto expr = parseExpr("x&&y");
    auto* andExpr = asBinary(expr.get());
    ASSERT_NE(andExpr, nullptr);
    EXPECT_EQ(andExpr->op(), BinaryOp::And);
    EXPECT_EQ(asIdent(&andExpr->lhs())->name(), "x");
    EXPECT_EQ(asIdent(&andExpr->rhs())->name(), "y");
}

// ─── Full precedence table ────────────────────────────────────────────────────

// a || b && c  →  BinaryExpr(Or, a, BinaryExpr(And, b, c))
// && binds tighter than ||
TEST(PrattTest, AndBindsTighterThanOr) {
    auto expr = parseExpr("a || b && c");
    auto* orExpr = asBinary(expr.get());
    ASSERT_NE(orExpr, nullptr);
    EXPECT_EQ(orExpr->op(), BinaryOp::Or);

    auto* andExpr = asBinary(&orExpr->rhs());
    ASSERT_NE(andExpr, nullptr);
    EXPECT_EQ(andExpr->op(), BinaryOp::And);
}

// a == b && c  →  BinaryExpr(And, BinaryExpr(Eq, a, b), c)
// == binds tighter than &&
TEST(PrattTest, EqBindsTighterThanAnd) {
    auto expr = parseExpr("a == b && c");
    auto* andExpr = asBinary(expr.get());
    ASSERT_NE(andExpr, nullptr);
    EXPECT_EQ(andExpr->op(), BinaryOp::And);

    auto* eq = asBinary(&andExpr->lhs());
    ASSERT_NE(eq, nullptr);
    EXPECT_EQ(eq->op(), BinaryOp::Eq);
}

// a < b == c < d  →  BinaryExpr(Eq, BinaryExpr(Lt,a,b), BinaryExpr(Lt,c,d))
// relational binds tighter than equality
TEST(PrattTest, RelationalBindsTighterThanEquality) {
    auto expr = parseExpr("a < b == c < d");
    auto* eq = asBinary(expr.get());
    ASSERT_NE(eq, nullptr);
    EXPECT_EQ(eq->op(), BinaryOp::Eq);
    EXPECT_EQ(asBinary(&eq->lhs())->op(), BinaryOp::Lt);
    EXPECT_EQ(asBinary(&eq->rhs())->op(), BinaryOp::Lt);
}

// a + b * c - d  →  BinaryExpr(Sub, BinaryExpr(Add,a,BinaryExpr(Mul,b,c)), d)
// Full arithmetic precedence
TEST(PrattTest, FullArithmeticPrecedence) {
    auto expr = parseExpr("a + b * c - d");
    // Expect left-associative:  (a + (b*c)) - d
    auto* sub = asBinary(expr.get());
    ASSERT_NE(sub, nullptr);
    EXPECT_EQ(sub->op(), BinaryOp::Sub);
    EXPECT_EQ(asIdent(&sub->rhs())->name(), "d");

    auto* add = asBinary(&sub->lhs());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op(), BinaryOp::Add);

    auto* mul = asBinary(&add->rhs());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op(), BinaryOp::Mul);
}

// ─── Left-associativity ───────────────────────────────────────────────────────

// 1 - 2 - 3  →  BinaryExpr(Sub, BinaryExpr(Sub,1,2), 3)
TEST(PrattTest, SubtractionIsLeftAssociative) {
    auto expr = parseExpr("1 - 2 - 3");
    auto* outer = asBinary(expr.get());
    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->op(), BinaryOp::Sub);
    EXPECT_EQ(asInt(&outer->rhs())->value(), 3);

    auto* inner = asBinary(&outer->lhs());
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->op(), BinaryOp::Sub);
    EXPECT_EQ(asInt(&inner->lhs())->value(), 1);
    EXPECT_EQ(asInt(&inner->rhs())->value(), 2);
}

// 8 / 4 / 2  →  ((8/4)/2) = 1
TEST(PrattTest, DivisionIsLeftAssociative) {
    auto expr = parseExpr("8 / 4 / 2");
    auto* outer = asBinary(expr.get());
    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->op(), BinaryOp::Div);
    auto* inner = asBinary(&outer->lhs());
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->op(), BinaryOp::Div);
}

// ─── Unary operators ─────────────────────────────────────────────────────────

// -x  →  UnaryExpr(Neg, x)
TEST(PrattTest, UnaryNegation) {
    auto expr = parseExpr("-x");
    auto* neg = asUnary(expr.get());
    ASSERT_NE(neg, nullptr);
    EXPECT_EQ(neg->op(), UnaryOp::Neg);
    EXPECT_EQ(asIdent(&neg->operand())->name(), "x");
}

// !x  →  UnaryExpr(Not, x)
TEST(PrattTest, LogicalNot) {
    auto expr = parseExpr("!x");
    auto* notExpr = asUnary(expr.get());
    ASSERT_NE(notExpr, nullptr);
    EXPECT_EQ(notExpr->op(), UnaryOp::Not);
}

// -1 + 2  →  BinaryExpr(Add, UnaryExpr(Neg, 1), 2)
// Unary minus binds tighter than addition.
TEST(PrattTest, UnaryMinusBindsTighterThanAdd) {
    auto expr = parseExpr("-1 + 2");
    auto* add = asBinary(expr.get());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op(), BinaryOp::Add);
    auto* neg = asUnary(&add->lhs());
    ASSERT_NE(neg, nullptr);
    EXPECT_EQ(neg->op(), UnaryOp::Neg);
}

// -(a + b)  →  UnaryExpr(Neg, BinaryExpr(Add, a, b))
TEST(PrattTest, UnaryOnGroupedExpr) {
    auto expr = parseExpr("-(a + b)");
    auto* neg = asUnary(expr.get());
    ASSERT_NE(neg, nullptr);
    EXPECT_EQ(neg->op(), UnaryOp::Neg);
    auto* add = asBinary(&neg->operand());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op(), BinaryOp::Add);
}

// ─── Literal types produce LiteralExpr base ───────────────────────────────────

TEST(PrattTest, IntLiteralIsLiteralExpr) {
    auto expr = parseExpr("42");
    EXPECT_NE(asLiteral(expr.get()), nullptr);
    EXPECT_NE(asInt(expr.get()), nullptr);
    EXPECT_EQ(asInt(expr.get())->value(), 42);
}

TEST(PrattTest, FloatLiteralIsLiteralExpr) {
    auto expr = parseExpr("3.14");
    EXPECT_NE(asLiteral(expr.get()), nullptr);
    EXPECT_NE(dynamic_cast<FloatLiteralExpr*>(expr.get()), nullptr);
}

TEST(PrattTest, StringLiteralIsLiteralExpr) {
    auto expr = parseExpr(R"("hello")");
    EXPECT_NE(asLiteral(expr.get()), nullptr);
}

TEST(PrattTest, BoolLiteralIsLiteralExpr) {
    auto expr = parseExpr("true");
    EXPECT_NE(asLiteral(expr.get()), nullptr);
    EXPECT_EQ(dynamic_cast<BoolLiteralExpr*>(expr.get())->value(), true);
}

// ─── parseProgram API ─────────────────────────────────────────────────────────

TEST(PrattTest, ParseProgramReturnsModule) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", "module demo\nfn main() {}");
    Lexer lex(ctx, id);
    Parser parser(ctx, lex.tokenize());
    auto mod = parser.parseProgram();
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->name(), "demo");
}

// ─── parseStatement API ───────────────────────────────────────────────────────

TEST(PrattTest, ParseStatementReturnStmt) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", "return 42");
    Lexer lex(ctx, id);
    Parser parser(ctx, lex.tokenize());
    // Skip the implicit 'module' declaration step; parseStatement works
    // at any position in the token stream.
    auto stmt = parser.parseStatement();
    ASSERT_NE(stmt, nullptr);
    auto* ret = dynamic_cast<ReturnStmt*>(stmt.get());
    ASSERT_NE(ret, nullptr);
    ASSERT_NE(ret->value(), nullptr);
    EXPECT_EQ(asInt(ret->value())->value(), 42);
}

TEST(PrattTest, ParseStatementIfStmt) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", "if x { return 1 }");
    Lexer lex(ctx, id);
    Parser parser(ctx, lex.tokenize());
    auto stmt = parser.parseStatement();
    ASSERT_NE(stmt, nullptr);
    EXPECT_NE(dynamic_cast<IfStmt*>(stmt.get()), nullptr);
}

TEST(PrattTest, ParseStatementWhileStmt) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", "while n > 0 { n -= 1 }");
    Lexer lex(ctx, id);
    Parser parser(ctx, lex.tokenize());
    auto stmt = parser.parseStatement();
    ASSERT_NE(stmt, nullptr);
    EXPECT_NE(dynamic_cast<WhileStmt*>(stmt.get()), nullptr);
}

// ─── Complex expression trees ────────────────────────────────────────────────

// a + b * c + d  →  (a + (b*c)) + d
TEST(PrattTest, ThreeTermWithMulInMiddle) {
    auto expr = parseExpr("a + b * c + d");
    auto* add2 = asBinary(expr.get());
    ASSERT_NE(add2, nullptr);
    EXPECT_EQ(add2->op(), BinaryOp::Add);
    EXPECT_EQ(asIdent(&add2->rhs())->name(), "d");

    auto* add1 = asBinary(&add2->lhs());
    ASSERT_NE(add1, nullptr);
    EXPECT_EQ(add1->op(), BinaryOp::Add);
    EXPECT_EQ(asBinary(&add1->rhs())->op(), BinaryOp::Mul);
}

// a != b || c < d  →  Or(NotEq(a,b), Lt(c,d))
TEST(PrattTest, ComparisonCombinedWithOr) {
    auto expr = parseExpr("a != b || c < d");
    auto* orExpr = asBinary(expr.get());
    ASSERT_NE(orExpr, nullptr);
    EXPECT_EQ(orExpr->op(), BinaryOp::Or);
    EXPECT_EQ(asBinary(&orExpr->lhs())->op(), BinaryOp::NotEq);
    EXPECT_EQ(asBinary(&orExpr->rhs())->op(), BinaryOp::Lt);
}

// !x && y  →  And(Not(x), y)   — unary ! tighter than &&
TEST(PrattTest, LogicalNotWithAnd) {
    auto expr = parseExpr("!x && y");
    auto* andExpr = asBinary(expr.get());
    ASSERT_NE(andExpr, nullptr);
    EXPECT_EQ(andExpr->op(), BinaryOp::And);
    EXPECT_EQ(asUnary(&andExpr->lhs())->op(), UnaryOp::Not);
}

