#include <gtest/gtest.h>

#include "vcc/common/CompilerContext.h"
#include "vcc/lexer/Lexer.h"
#include "vcc/parser/Parser.h"
#include "vcc/semantic/SemanticAnalyzer.h"

using namespace vcc::common;
using namespace vcc::lexer;
using namespace vcc::parser;
using namespace vcc::semantic;

// ─── Helpers ─────────────────────────────────────────────────────────────────

struct SemaResult {
    bool            ok;
    uint32_t        errors;
    uint32_t        warnings;
};

static SemaResult analyze(const std::string& src) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", src);
    Lexer lex(ctx, id);
    Parser parser(ctx, lex.tokenize());
    auto mod = parser.parse();
    if (!mod) return {false, ctx.diagnostics().errorCount(), 0};

    SemanticAnalyzer sema(ctx);
    bool ok = sema.analyze(*mod);
    return {ok,
            ctx.diagnostics().errorCount(),
            ctx.diagnostics().warningCount()};
}

// ─── Symbol table ─────────────────────────────────────────────────────────────

TEST(SemanticTest, SymbolTableBasics) {
    SymbolTable tbl;
    EXPECT_EQ(tbl.depth(), 0u);

    tbl.enterScope("inner");
    EXPECT_EQ(tbl.depth(), 1u);

    Symbol s{"x", SymbolKind::Variable, true, false};
    EXPECT_TRUE(tbl.define(s));
    EXPECT_FALSE(tbl.define(s));  // redefinition → false

    EXPECT_NE(tbl.lookup("x"), nullptr);
    tbl.leaveScope();
    EXPECT_EQ(tbl.lookup("x"), nullptr);
}

TEST(SemanticTest, SymbolShadowing) {
    SymbolTable tbl;
    Symbol outer{"x", SymbolKind::Variable, false, false};
    tbl.define(outer);

    tbl.enterScope("inner");
    Symbol inner{"x", SymbolKind::Variable, true, false};
    tbl.define(inner);

    const Symbol* found = tbl.lookup("x");
    ASSERT_NE(found, nullptr);
    EXPECT_TRUE(found->isMut);

    tbl.leaveScope();
    found = tbl.lookup("x");
    ASSERT_NE(found, nullptr);
    EXPECT_FALSE(found->isMut);
}

// ─── Name resolution ─────────────────────────────────────────────────────────

TEST(SemanticTest, UndeclaredVariable) {
    auto r = analyze("fn f() { let y = x }");  // x undeclared
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, DeclaredVariable) {
    auto r = analyze("fn f() { let x = 1 let y = x }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, FunctionForwardReference) {
    // Both functions are pre-declared so mutual recursion should work
    auto r = analyze(
        "fn even(n: i64) -> bool { return odd(n) }\n"
        "fn odd(n: i64) -> bool { return even(n) }\n"
    );
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, RedefinitionError) {
    auto r = analyze("fn f() {}\nfn f() {}");
    EXPECT_GT(r.errors, 0u);
}

// ─── Import ───────────────────────────────────────────────────────────────────

TEST(SemanticTest, ImportAddsSymbol) {
    auto r = analyze("import std.io\nfn f() {}");
    EXPECT_EQ(r.errors, 0u);
}

// ─── Loops ────────────────────────────────────────────────────────────────────

TEST(SemanticTest, ForLoopVariable) {
    // The loop variable 'i' should be visible inside the body
    auto r = analyze("fn f() { for i in items { let k = i } }");
    // items is undeclared but i should be resolvable; 1 error expected (items)
    EXPECT_LE(r.errors, 1u);
}

// ─── Type checker ────────────────────────────────────────────────────────────

TEST(SemanticTest, ValidReturnType) {
    auto r = analyze("fn square(x: i64) -> i64 { return x }");
    EXPECT_EQ(r.errors, 0u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Check 1 – Variable declared before use
// ═══════════════════════════════════════════════════════════════════════════════

TEST(SemanticTest, UseBeforeDecl_Error) {
    // 'y' used before it is declared
    auto r = analyze("fn f() { let x = y }");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, UseAfterDecl_OK) {
    auto r = analyze("fn f() { let x = 1 let y = x }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, UseInNestedScope_OK) {
    auto r = analyze("fn f() { let x = 1 if true { let y = x } }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, UseOuterScopeFromInner_OK) {
    // Inner block can read outer-scope variable.
    auto r = analyze(
        "fn f() {\n"
        "  let counter = 0\n"
        "  while true { let tmp = counter }\n"
        "}\n"
    );
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, UndeclaredInBinaryExpr_Error) {
    auto r = analyze("fn f() { let x = a + 1 }");
    EXPECT_GT(r.errors, 0u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Check 2 – Duplicate variable declarations
// ═══════════════════════════════════════════════════════════════════════════════

TEST(SemanticTest, DuplicateVarInSameScope_Error) {
    auto r = analyze("fn f() { let x = 1 let x = 2 }");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, DuplicateFunctionDecl_Error) {
    auto r = analyze("fn f() {}\nfn f() {}");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, ShadowingInNestedScope_OK) {
    // Redeclaring in a nested scope is allowed (shadowing).
    auto r = analyze("fn f() { let x = 1 if true { let x = 2 } }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, DuplicateParameter_Error) {
    auto r = analyze("fn f(a: i64, a: i64) {}");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, DuplicateDiagnosticHasNote) {
    // Two errors raised: the redefinition error + note about prior definition.
    // At minimum one error must be emitted.
    auto r = analyze("fn g() {}\nfn g() {}");
    EXPECT_GE(r.errors, 1u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Check 3 – Type checking
// ═══════════════════════════════════════════════════════════════════════════════

TEST(SemanticTest, IntLiteralType) {
    // No type error: int literal fits i64 annotation.
    auto r = analyze("fn f() { let x: i64 = 42 }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, StringLiteralType) {
    auto r = analyze("fn f() { let s: str = \"hello\" }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, BoolLiteralType) {
    auto r = analyze("fn f() { let b: bool = true }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, TypeMismatchInInit_Error) {
    // Declaring i64 but assigning string literal.
    auto r = analyze("fn f() { let x: i64 = \"oops\" }");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, TypeMismatchInBinary_Error) {
    // Adding an integer literal to a string literal.
    auto r = analyze("fn f() { let x = 1 + \"hello\" }");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, CompatibleReturnType_OK) {
    auto r = analyze("fn add(a: i64, b: i64) -> i64 { return a }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, ReturnTypeMismatch_Error) {
    // Function returns i64 but return expression is a string.
    auto r = analyze("fn f() -> i64 { return \"bad\" }");
    EXPECT_GT(r.errors, 0u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Check 4 – Function existence
// ═══════════════════════════════════════════════════════════════════════════════

TEST(SemanticTest, CallUndeclaredFunction_Error) {
    auto r = analyze("fn f() { ghost() }");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, CallDeclaredFunction_OK) {
    auto r = analyze("fn helper() {}\nfn f() { helper() }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, CallForwardDeclaredFunction_OK) {
    // 'early' is declared AFTER 'caller'; Pass 1 hoists it.
    auto r = analyze("fn caller() { early() }\nfn early() {}");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, CallNonFunction_Error) {
    // 'x' is a variable, not a function.
    auto r = analyze("fn f() { let x = 1 x() }");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, CallWrongArgCount_Error) {
    auto r = analyze("fn add(a: i64, b: i64) -> i64 { return a }\nfn f() { add(1) }");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, CallCorrectArgCount_OK) {
    auto r = analyze("fn add(a: i64, b: i64) -> i64 { return a }\nfn f() { add(1, 2) }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, MutualRecursion_OK) {
    auto r = analyze(
        "fn even(n: i64) -> bool { return odd(n) }\n"
        "fn odd(n: i64) -> bool { return even(n) }\n"
    );
    EXPECT_EQ(r.errors, 0u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Check 5 – Return statement validation
// ═══════════════════════════════════════════════════════════════════════════════

TEST(SemanticTest, MissingReturn_Error) {
    // Function declares i64 return but has no return statement.
    auto r = analyze("fn f() -> i64 {}");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, VoidFunctionNoReturn_OK) {
    // Unit function with no return — perfectly valid.
    auto r = analyze("fn f() {}");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, EmptyReturnInVoidFunction_OK) {
    auto r = analyze("fn f() { return }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, EmptyReturnInNonVoidFunction_Error) {
    // 'return' with no value in a function that declares i64 return.
    auto r = analyze("fn f() -> i64 { return }");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, ReturnWithValue_OK) {
    auto r = analyze("fn f() -> i64 { return 42 }");
    EXPECT_EQ(r.errors, 0u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Check 6 – Scope validation
// ═══════════════════════════════════════════════════════════════════════════════

TEST(SemanticTest, OuterVarNotVisibleAfterBlock) {
    // Variable declared inside an if block is not accessible after the block.
    auto r = analyze("fn f() { if true { let inner = 1 } let y = inner }");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, ConstReassignment_Error) {
    auto r = analyze("fn f() { const C = 1 C = 2 }");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, LetReassignment_Error) {
    // 'let' is immutable; reassignment must be rejected.
    auto r = analyze("fn f() { let x = 1 x = 2 }");
    EXPECT_GT(r.errors, 0u);
}

TEST(SemanticTest, VarReassignment_OK) {
    // 'var' is mutable; reassignment is allowed.
    auto r = analyze("fn f() { var x = 1 x = 2 }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, ForLoopVarInScope_OK) {
    // Loop variable is visible inside the for body.
    auto r = analyze("fn f() { let items = 0 for i in items { let k = i } }");
    EXPECT_EQ(r.errors, 0u);
}

TEST(SemanticTest, NestedScopeIndependence) {
    // Same name in two sibling scopes — each if branch is a separate scope.
    auto r = analyze(
        "fn f() {\n"
        "  if true { let x = 1 }\n"
        "  if true { let x = 2 }\n"
        "}\n"
    );
    EXPECT_EQ(r.errors, 0u);
}
