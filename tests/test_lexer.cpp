#include <gtest/gtest.h>

#include "vcc/common/CompilerContext.h"
#include "vcc/lexer/Lexer.h"

using namespace vcc::common;
using namespace vcc::lexer;

// ─── Helpers ─────────────────────────────────────────────────────────────────

/// Lex via the context-based constructor (original path).
static std::vector<Token> lex(const std::string& src) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", src);
    Lexer lexer(ctx, id);
    auto tokens = lexer.tokenize();
    if (!tokens.empty() && tokens.back().isEof()) tokens.pop_back();
    return tokens;
}

/// Lex via the convenience string constructor (new path).
static std::vector<Token> lexStr(const std::string& src) {
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    if (!tokens.empty() && tokens.back().isEof()) tokens.pop_back();
    return tokens;
}

// ═══════════════════════════════════════════════════════════════════════════
// 1. String-based constructor
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerStringCtorTest, ProducesTokens) {
    auto toks = lexStr("let x = 42");
    ASSERT_EQ(toks.size(), 4u);
    EXPECT_EQ(toks[0].kind, TokenKind::KwLet);
    EXPECT_EQ(toks[1].kind, TokenKind::Identifier);
    EXPECT_EQ(toks[2].kind, TokenKind::Eq);
    EXPECT_EQ(toks[3].kind, TokenKind::IntLiteral);
}

TEST(LexerStringCtorTest, EmptySourceGivesNoTokens) {
    auto toks = lexStr("");
    EXPECT_TRUE(toks.empty());
}

TEST(LexerStringCtorTest, DiagnosticsAccessible) {
    Lexer lexer("42");
    (void)lexer.tokenize();
    EXPECT_EQ(lexer.diagnostics().errorCount(), 0u);
}

TEST(LexerStringCtorTest, ErrorDiagnosticEmitted) {
    Lexer lexer("42 $$ 99");
    (void)lexer.tokenize();
    EXPECT_GT(lexer.diagnostics().errorCount(), 0u);
}

// ═══════════════════════════════════════════════════════════════════════════
// 2. TokenType alias
// ═══════════════════════════════════════════════════════════════════════════

TEST(TokenTypeAliasTest, CanUseTokenTypeAlias) {
    auto toks = lex("let");
    ASSERT_EQ(toks.size(), 1u);
    // TokenType is an alias for TokenKind — both should compile and be equal
    TokenType tk = toks[0].kind;
    EXPECT_EQ(tk, TokenType::KwLet);
}

// ═══════════════════════════════════════════════════════════════════════════
// 3. tokenTypeName – uppercase display names
// ═══════════════════════════════════════════════════════════════════════════

TEST(TokenTypeNameTest, LiteralsHaveCorrectNames) {
    EXPECT_EQ(tokenTypeName(TokenKind::IntLiteral),    "INTEGER");
    EXPECT_EQ(tokenTypeName(TokenKind::FloatLiteral),  "FLOAT");
    EXPECT_EQ(tokenTypeName(TokenKind::StringLiteral), "STRING");
    EXPECT_EQ(tokenTypeName(TokenKind::CharLiteral),   "CHAR");
}

TEST(TokenTypeNameTest, IdentifierName) {
    EXPECT_EQ(tokenTypeName(TokenKind::Identifier), "IDENTIFIER");
}

TEST(TokenTypeNameTest, KeywordsHaveCorrectNames) {
    EXPECT_EQ(tokenTypeName(TokenKind::KwFn),       "FN");
    EXPECT_EQ(tokenTypeName(TokenKind::KwLet),      "LET");
    EXPECT_EQ(tokenTypeName(TokenKind::KwVar),      "VAR");
    EXPECT_EQ(tokenTypeName(TokenKind::KwConst),    "CONST");
    EXPECT_EQ(tokenTypeName(TokenKind::KwIf),       "IF");
    EXPECT_EQ(tokenTypeName(TokenKind::KwElse),     "ELSE");
    EXPECT_EQ(tokenTypeName(TokenKind::KwWhile),    "WHILE");
    EXPECT_EQ(tokenTypeName(TokenKind::KwFor),      "FOR");
    EXPECT_EQ(tokenTypeName(TokenKind::KwReturn),   "RETURN");
    EXPECT_EQ(tokenTypeName(TokenKind::KwBreak),    "BREAK");
    EXPECT_EQ(tokenTypeName(TokenKind::KwContinue), "CONTINUE");
    EXPECT_EQ(tokenTypeName(TokenKind::KwTrue),     "TRUE");
    EXPECT_EQ(tokenTypeName(TokenKind::KwFalse),    "FALSE");
}

TEST(TokenTypeNameTest, OperatorsHaveCorrectNames) {
    EXPECT_EQ(tokenTypeName(TokenKind::Eq),      "ASSIGN");
    EXPECT_EQ(tokenTypeName(TokenKind::Plus),    "PLUS");
    EXPECT_EQ(tokenTypeName(TokenKind::Minus),   "MINUS");
    EXPECT_EQ(tokenTypeName(TokenKind::Star),    "STAR");
    EXPECT_EQ(tokenTypeName(TokenKind::Slash),   "SLASH");
    EXPECT_EQ(tokenTypeName(TokenKind::Percent), "PERCENT");
    EXPECT_EQ(tokenTypeName(TokenKind::EqEq),    "EQEQ");
    EXPECT_EQ(tokenTypeName(TokenKind::NotEq),   "BANG_EQ");
    EXPECT_EQ(tokenTypeName(TokenKind::Lt),      "LT");
    EXPECT_EQ(tokenTypeName(TokenKind::Gt),      "GT");
    EXPECT_EQ(tokenTypeName(TokenKind::LtEq),    "LT_EQ");
    EXPECT_EQ(tokenTypeName(TokenKind::GtEq),    "GT_EQ");
    EXPECT_EQ(tokenTypeName(TokenKind::AndAnd),  "AND_AND");
    EXPECT_EQ(tokenTypeName(TokenKind::OrOr),    "OR_OR");
    EXPECT_EQ(tokenTypeName(TokenKind::Bang),    "BANG");
    EXPECT_EQ(tokenTypeName(TokenKind::PlusEq),  "PLUS_EQ");
    EXPECT_EQ(tokenTypeName(TokenKind::MinusEq), "MINUS_EQ");
    EXPECT_EQ(tokenTypeName(TokenKind::StarEq),  "STAR_EQ");
    EXPECT_EQ(tokenTypeName(TokenKind::SlashEq), "SLASH_EQ");
}

TEST(TokenTypeNameTest, PunctuationHasCorrectNames) {
    EXPECT_EQ(tokenTypeName(TokenKind::LParen),    "LPAREN");
    EXPECT_EQ(tokenTypeName(TokenKind::RParen),    "RPAREN");
    EXPECT_EQ(tokenTypeName(TokenKind::LBrace),    "LBRACE");
    EXPECT_EQ(tokenTypeName(TokenKind::RBrace),    "RBRACE");
    EXPECT_EQ(tokenTypeName(TokenKind::LBracket),  "LBRACKET");
    EXPECT_EQ(tokenTypeName(TokenKind::RBracket),  "RBRACKET");
    EXPECT_EQ(tokenTypeName(TokenKind::Comma),     "COMMA");
    EXPECT_EQ(tokenTypeName(TokenKind::Colon),     "COLON");
    EXPECT_EQ(tokenTypeName(TokenKind::Arrow),     "ARROW");
    EXPECT_EQ(tokenTypeName(TokenKind::Dot),       "DOT");
}

TEST(TokenTypeNameTest, SpecialTokenNames) {
    EXPECT_EQ(tokenTypeName(TokenKind::Eof),   "EOF");
    EXPECT_EQ(tokenTypeName(TokenKind::Error), "ERROR");
}

// ═══════════════════════════════════════════════════════════════════════════
// 4. Integer literals
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, IntegerDecimal) {
    auto toks = lex("42");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::IntLiteral);
    EXPECT_EQ(toks[0].lexeme, "42");
}

TEST(LexerTest, IntegerZero) {
    auto toks = lex("0");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::IntLiteral);
    EXPECT_EQ(toks[0].lexeme, "0");
}

TEST(LexerTest, IntegerLargeValue) {
    auto toks = lex("1000000");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::IntLiteral);
    EXPECT_EQ(toks[0].lexeme, "1000000");
}

TEST(LexerTest, HexLiteral) {
    auto toks = lex("0xFF");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::IntLiteral);
    EXPECT_EQ(toks[0].lexeme, "0xFF");
}

TEST(LexerTest, HexLiteralLowercase) {
    auto toks = lex("0xdeadbeef");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::IntLiteral);
}

TEST(LexerTest, BinaryLiteral) {
    auto toks = lex("0b1010");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::IntLiteral);
    EXPECT_EQ(toks[0].lexeme, "0b1010");
}

TEST(LexerTest, OctalLiteral) {
    auto toks = lex("0o777");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::IntLiteral);
}

// ═══════════════════════════════════════════════════════════════════════════
// 5. Float literals
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, FloatSimple) {
    auto toks = lex("3.14");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::FloatLiteral);
    EXPECT_EQ(toks[0].lexeme, "3.14");
}

TEST(LexerTest, FloatWithExponent) {
    auto toks = lex("1.0e9");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::FloatLiteral);
}

TEST(LexerTest, FloatWithNegativeExponent) {
    auto toks = lex("6.022e-23");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::FloatLiteral);
}

TEST(LexerTest, FloatWithPositiveExponent) {
    auto toks = lex("1.5e+10");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::FloatLiteral);
}

// ═══════════════════════════════════════════════════════════════════════════
// 6. String literals
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, StringLiteralSimple) {
    auto toks = lex(R"("hello world")");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::StringLiteral);
}

TEST(LexerTest, StringLiteralEmpty) {
    auto toks = lex(R"("")");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::StringLiteral);
    EXPECT_EQ(toks[0].lexeme, "\"\"");
}

TEST(LexerTest, StringLiteralWithEscapes) {
    auto toks = lex(R"("line\nnewline\ttab\"quote")");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::StringLiteral);
}

// ═══════════════════════════════════════════════════════════════════════════
// 7. Char literals
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, CharLiteralSimple) {
    auto toks = lex("'a'");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::CharLiteral);
    EXPECT_EQ(toks[0].lexeme, "'a'");
}

TEST(LexerTest, CharLiteralEscape) {
    auto toks = lex("'\\n'");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::CharLiteral);
}

// ═══════════════════════════════════════════════════════════════════════════
// 8. Keywords
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, CoreKeywords) {
    auto toks = lex("fn let var const if else while for return");
    ASSERT_EQ(toks.size(), 9u);
    EXPECT_EQ(toks[0].kind, TokenKind::KwFn);
    EXPECT_EQ(toks[1].kind, TokenKind::KwLet);
    EXPECT_EQ(toks[2].kind, TokenKind::KwVar);
    EXPECT_EQ(toks[3].kind, TokenKind::KwConst);
    EXPECT_EQ(toks[4].kind, TokenKind::KwIf);
    EXPECT_EQ(toks[5].kind, TokenKind::KwElse);
    EXPECT_EQ(toks[6].kind, TokenKind::KwWhile);
    EXPECT_EQ(toks[7].kind, TokenKind::KwFor);
    EXPECT_EQ(toks[8].kind, TokenKind::KwReturn);
}

TEST(LexerTest, BoolKeywords) {
    auto toks = lex("true false");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].kind, TokenKind::KwTrue);
    EXPECT_EQ(toks[1].kind, TokenKind::KwFalse);
}

TEST(LexerTest, ModuleImportKeywords) {
    auto toks = lex("module import");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].kind, TokenKind::KwModule);
    EXPECT_EQ(toks[1].kind, TokenKind::KwImport);
}

TEST(LexerTest, StructEnumTypeKeywords) {
    auto toks = lex("struct enum type interface");
    ASSERT_EQ(toks.size(), 4u);
    EXPECT_EQ(toks[0].kind, TokenKind::KwStruct);
    EXPECT_EQ(toks[1].kind, TokenKind::KwEnum);
    EXPECT_EQ(toks[2].kind, TokenKind::KwType);
    EXPECT_EQ(toks[3].kind, TokenKind::KwInterface);
}

TEST(LexerTest, ControlFlowKeywords) {
    auto toks = lex("break continue match");
    ASSERT_EQ(toks.size(), 3u);
    EXPECT_EQ(toks[0].kind, TokenKind::KwBreak);
    EXPECT_EQ(toks[1].kind, TokenKind::KwContinue);
    EXPECT_EQ(toks[2].kind, TokenKind::KwMatch);
}

TEST(LexerTest, VisibilityMutabilityKeywords) {
    auto toks = lex("pub mut self as nil in");
    ASSERT_EQ(toks.size(), 6u);
    EXPECT_EQ(toks[0].kind, TokenKind::KwPub);
    EXPECT_EQ(toks[1].kind, TokenKind::KwMut);
    EXPECT_EQ(toks[2].kind, TokenKind::KwSelf);
    EXPECT_EQ(toks[3].kind, TokenKind::KwAs);
    EXPECT_EQ(toks[4].kind, TokenKind::KwNil);
    EXPECT_EQ(toks[5].kind, TokenKind::KwIn);
}

TEST(LexerTest, KeywordIsNotIdentifier) {
    auto toks = lex("fn");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::KwFn);
    EXPECT_NE(toks[0].kind, TokenKind::Identifier);
}

TEST(LexerTest, KeywordPrefixIsIdentifier) {
    // "letx" must be an identifier, not KwLet + KwX
    auto toks = lex("letx");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::Identifier);
    EXPECT_EQ(toks[0].lexeme, "letx");
}

// ═══════════════════════════════════════════════════════════════════════════
// 9. Identifiers
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, SimpleIdentifier) {
    auto toks = lex("my_variable");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::Identifier);
    EXPECT_EQ(toks[0].lexeme, "my_variable");
}

TEST(LexerTest, UnderscoreIdentifier) {
    auto toks = lex("_foo");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::Identifier);
}

TEST(LexerTest, UppercaseIdentifier) {
    auto toks = lex("MyStruct");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::Identifier);
    EXPECT_EQ(toks[0].lexeme, "MyStruct");
}

TEST(LexerTest, IdentifierWithNumbers) {
    auto toks = lex("x2");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::Identifier);
    EXPECT_EQ(toks[0].lexeme, "x2");
}

// ═══════════════════════════════════════════════════════════════════════════
// 10. Arithmetic operators
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, ArithmeticOps) {
    auto toks = lex("+ - * / %");
    ASSERT_EQ(toks.size(), 5u);
    EXPECT_EQ(toks[0].kind, TokenKind::Plus);
    EXPECT_EQ(toks[1].kind, TokenKind::Minus);
    EXPECT_EQ(toks[2].kind, TokenKind::Star);
    EXPECT_EQ(toks[3].kind, TokenKind::Slash);
    EXPECT_EQ(toks[4].kind, TokenKind::Percent);
}

// ═══════════════════════════════════════════════════════════════════════════
// 11. Comparison operators
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, ComparisonOps) {
    auto toks = lex("== != < > <= >=");
    ASSERT_EQ(toks.size(), 6u);
    EXPECT_EQ(toks[0].kind, TokenKind::EqEq);
    EXPECT_EQ(toks[1].kind, TokenKind::NotEq);
    EXPECT_EQ(toks[2].kind, TokenKind::Lt);
    EXPECT_EQ(toks[3].kind, TokenKind::Gt);
    EXPECT_EQ(toks[4].kind, TokenKind::LtEq);
    EXPECT_EQ(toks[5].kind, TokenKind::GtEq);
}

// ═══════════════════════════════════════════════════════════════════════════
// 12. Logical operators
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, LogicalOps) {
    auto toks = lex("&& || !");
    ASSERT_EQ(toks.size(), 3u);
    EXPECT_EQ(toks[0].kind, TokenKind::AndAnd);
    EXPECT_EQ(toks[1].kind, TokenKind::OrOr);
    EXPECT_EQ(toks[2].kind, TokenKind::Bang);
}

// ═══════════════════════════════════════════════════════════════════════════
// 13. Bitwise operators
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, BitwiseOps) {
    auto toks = lex("& | ^ ~ << >>");
    ASSERT_EQ(toks.size(), 6u);
    EXPECT_EQ(toks[0].kind, TokenKind::Amp);
    EXPECT_EQ(toks[1].kind, TokenKind::Pipe);
    EXPECT_EQ(toks[2].kind, TokenKind::Caret);
    EXPECT_EQ(toks[3].kind, TokenKind::Tilde);
    EXPECT_EQ(toks[4].kind, TokenKind::Shl);
    EXPECT_EQ(toks[5].kind, TokenKind::Shr);
}

// ═══════════════════════════════════════════════════════════════════════════
// 14. Assignment operators
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, AssignOp) {
    auto toks = lex("=");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].kind, TokenKind::Eq);
}

TEST(LexerTest, CompoundAssignOps) {
    auto toks = lex("+= -= *= /= %= &= |= ^=");
    ASSERT_EQ(toks.size(), 8u);
    EXPECT_EQ(toks[0].kind, TokenKind::PlusEq);
    EXPECT_EQ(toks[1].kind, TokenKind::MinusEq);
    EXPECT_EQ(toks[2].kind, TokenKind::StarEq);
    EXPECT_EQ(toks[3].kind, TokenKind::SlashEq);
    EXPECT_EQ(toks[4].kind, TokenKind::PercentEq);
    EXPECT_EQ(toks[5].kind, TokenKind::AmpEq);
    EXPECT_EQ(toks[6].kind, TokenKind::PipeEq);
    EXPECT_EQ(toks[7].kind, TokenKind::CaretEq);
}

// ═══════════════════════════════════════════════════════════════════════════
// 15. Punctuation
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, Punctuation) {
    auto toks = lex("( ) { } [ ] , ; : . ?");
    ASSERT_EQ(toks.size(), 11u);
    EXPECT_EQ(toks[0].kind, TokenKind::LParen);
    EXPECT_EQ(toks[1].kind, TokenKind::RParen);
    EXPECT_EQ(toks[2].kind, TokenKind::LBrace);
    EXPECT_EQ(toks[3].kind, TokenKind::RBrace);
    EXPECT_EQ(toks[4].kind, TokenKind::LBracket);
    EXPECT_EQ(toks[5].kind, TokenKind::RBracket);
    EXPECT_EQ(toks[6].kind, TokenKind::Comma);
    EXPECT_EQ(toks[7].kind, TokenKind::Semicolon);
    EXPECT_EQ(toks[8].kind, TokenKind::Colon);
    EXPECT_EQ(toks[9].kind, TokenKind::Dot);
    EXPECT_EQ(toks[10].kind, TokenKind::Question);
}

TEST(LexerTest, MultiCharPunctuation) {
    auto toks = lex(":: -> => .. ...");
    ASSERT_EQ(toks.size(), 5u);
    EXPECT_EQ(toks[0].kind, TokenKind::ColonColon);
    EXPECT_EQ(toks[1].kind, TokenKind::Arrow);
    EXPECT_EQ(toks[2].kind, TokenKind::FatArrow);
    EXPECT_EQ(toks[3].kind, TokenKind::DotDot);
    EXPECT_EQ(toks[4].kind, TokenKind::DotDotDot);
}

// ═══════════════════════════════════════════════════════════════════════════
// 16. Comments
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, LineCommentSkipped) {
    auto toks = lex("42 // this is a comment\n99");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].lexeme, "42");
    EXPECT_EQ(toks[1].lexeme, "99");
}

TEST(LexerTest, LineCommentAtEof) {
    auto toks = lex("42 // no newline at end");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].lexeme, "42");
}

TEST(LexerTest, BlockCommentSkipped) {
    auto toks = lex("1 /* block */ 2");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].lexeme, "1");
    EXPECT_EQ(toks[1].lexeme, "2");
}

TEST(LexerTest, NestedBlockComment) {
    auto toks = lex("1 /* outer /* inner */ outer */ 2");
    ASSERT_EQ(toks.size(), 2u);
}

TEST(LexerTest, BlockCommentSpanningLines) {
    auto toks = lex("a /* line1\nline2 */ b");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].lexeme, "a");
    EXPECT_EQ(toks[1].lexeme, "b");
}

// ═══════════════════════════════════════════════════════════════════════════
// 17. Source location tracking
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, FirstTokenIsLine1Col1) {
    auto toks = lex("hello");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].location.line,   1u);
    EXPECT_EQ(toks[0].location.column, 1u);
}

TEST(LexerTest, LocationTrackingAcrossLines) {
    auto toks = lex("x\ny");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].location.line, 1u);
    EXPECT_EQ(toks[1].location.line, 2u);
    EXPECT_EQ(toks[1].location.column, 1u);
}

TEST(LexerTest, ColumnTrackingOnOneLine) {
    auto toks = lex("a b c");
    ASSERT_EQ(toks.size(), 3u);
    EXPECT_EQ(toks[0].location.column, 1u);
    EXPECT_EQ(toks[1].location.column, 3u);
    EXPECT_EQ(toks[2].location.column, 5u);
}

TEST(LexerTest, MultiLineProgram) {
    const char* src = "fn main() {\n"
                      "    let x = 10\n"
                      "}\n";
    auto toks = lex(src);
    // fn  main  (  )  {  let  x  =  10  }
    EXPECT_GE(toks.size(), 10u);
    // 'fn' must be on line 1
    EXPECT_EQ(toks[0].location.line, 1u);
    // 'let' must be on line 2
    auto it = std::find_if(toks.begin(), toks.end(),
        [](const Token& t){ return t.kind == TokenKind::KwLet; });
    ASSERT_NE(it, toks.end());
    EXPECT_EQ(it->location.line, 2u);
}

// ═══════════════════════════════════════════════════════════════════════════
// 18. Error recovery
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, ErrorOnUnknownChar) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", "@$");
    Lexer lexer(ctx, id);
    // '@' is a valid V token; '$' should produce an error token
    auto toks = lexer.tokenize();
    // The error '$' must be reported as an Error token
    bool hasError = std::any_of(toks.begin(), toks.end(),
        [](const Token& t){ return t.kind == TokenKind::Error; });
    EXPECT_TRUE(hasError);
    EXPECT_GT(ctx.diagnostics().errorCount(), 0u);
}

TEST(LexerTest, LexingContinuesAfterError) {
    auto toks = lex("let $ x");
    // After the bad '$', the lexer should still produce x as an identifier
    bool hasIdent = std::any_of(toks.begin(), toks.end(),
        [](const Token& t){ return t.kind == TokenKind::Identifier && t.lexeme == "x"; });
    EXPECT_TRUE(hasIdent);
}

// ═══════════════════════════════════════════════════════════════════════════
// 19. Full program tokenisation
// ═══════════════════════════════════════════════════════════════════════════

TEST(LexerTest, FullVariableDecl) {
    // let x = 100
    auto toks = lex("let x = 100");
    ASSERT_EQ(toks.size(), 4u);
    EXPECT_EQ(toks[0].kind,   TokenKind::KwLet);
    EXPECT_EQ(toks[0].lexeme, "let");
    EXPECT_EQ(toks[1].kind,   TokenKind::Identifier);
    EXPECT_EQ(toks[1].lexeme, "x");
    EXPECT_EQ(toks[2].kind,   TokenKind::Eq);
    EXPECT_EQ(toks[2].lexeme, "=");
    EXPECT_EQ(toks[3].kind,   TokenKind::IntLiteral);
    EXPECT_EQ(toks[3].lexeme, "100");
}

TEST(LexerTest, FullFunctionDecl) {
    const char* src = "fn add(a: int, b: int) -> int { return a + b }";
    auto toks = lex(src);
    EXPECT_EQ(toks[0].kind, TokenKind::KwFn);
    EXPECT_EQ(toks[1].kind, TokenKind::Identifier);   // add
    EXPECT_EQ(toks[2].kind, TokenKind::LParen);
    // last meaningful token before EOF should be }
    EXPECT_EQ(toks.back().kind, TokenKind::RBrace);
}

TEST(LexerTest, TokenTypeNamesMatchLexedProgram) {
    // Verify the tokenTypeName() of actual lexed tokens
    auto toks = lex("let x = 100");
    EXPECT_EQ(tokenTypeName(toks[0].kind), "LET");
    EXPECT_EQ(tokenTypeName(toks[1].kind), "IDENTIFIER");
    EXPECT_EQ(tokenTypeName(toks[2].kind), "ASSIGN");
    EXPECT_EQ(tokenTypeName(toks[3].kind), "INTEGER");
}

TEST(LexerTest, EofTokenHasTypeName) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", "42");
    Lexer lexer(ctx, id);
    auto toks = lexer.tokenize();
    ASSERT_FALSE(toks.empty());
    EXPECT_EQ(toks.back().kind, TokenKind::Eof);
    EXPECT_EQ(tokenTypeName(toks.back().kind), "EOF");
}

