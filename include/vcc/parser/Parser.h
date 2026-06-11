#pragma once

#include "vcc/ast/Declarations.h"
#include "vcc/ast/Expressions.h"
#include "vcc/ast/Statements.h"
#include "vcc/ast/Types.h"
#include "vcc/common/CompilerContext.h"
#include "vcc/common/Token.h"

#include <memory>
#include <span>
#include <vector>

/// @file Parser.h
/// VCC recursive-descent / Pratt-parser for the V language.
///
/// Design
/// ──────
///  • Recursive descent for declarations and statements.
///  • Pratt (top-down operator-precedence) parser for expressions.
///  • Error recovery: on a syntax error the parser emits a diagnostic and
///    attempts to synchronise at the next statement boundary (`;` or `}`).
///  • No backtracking; single token look-ahead with peek(n) where needed.
///
/// Ownership
/// ─────────
///  The parser is a single-use, move-only object constructed with a token
///  vector produced by the Lexer.  Calling parseProgram() returns the root
///  ModuleDecl that owns the entire AST.
///
/// Pratt expression precedence table (higher = tighter binding)
/// ─────────────────────────────────────────────────────────────
///   1   ||          logical OR
///   2   &&          logical AND
///   3   |           bitwise OR
///   4   ^           bitwise XOR
///   5   &           bitwise AND
///   6   == !=       equality
///   7   < > <= >=   relational
///   8   << >>       shifts
///   9   + -         additive
///  10   * / %       multiplicative
///  Prefix unary: - ! ~ * &  (parsed before any infix)
///  Postfix:      ()  []  .  as  (tightest, parsed after primary)
///
/// Future extensions
/// ─────────────────
///  • Generalised type parameters:  `fn foo<T>(x: T) -> T`.
///  • Where clauses / interface bounds.
///  • Match expressions.
///  • Right-associative operators (** for exponentiation, etc.).

namespace vcc::parser {

class Parser {
public:
    /// Construct a parser.  @p tokens must include a trailing Eof token.
    Parser(common::CompilerContext&   ctx,
           std::vector<common::Token> tokens);

    // Single-use; move only.
    Parser(const Parser&)            = delete;
    Parser& operator=(const Parser&) = delete;
    Parser(Parser&&)                 = default;
    Parser& operator=(Parser&&)      = default;

    /// Parse the full token stream and return the module's AST root.
    /// Returns nullptr if a fatal parse error occurred.
    /// Alias: parseProgram() is the canonical public name for this method.
    [[nodiscard]] std::unique_ptr<ast::ModuleDecl> parse();

    /// Parse the full token stream and return the module's AST root.
    /// Equivalent to parse(); provided for requirement-spec API compatibility.
    [[nodiscard]] std::unique_ptr<ast::ModuleDecl> parseProgram() { return parse(); }

    /// Parse a single statement from the current token position.
    /// Useful for unit testing individual statements without a full program.
    [[nodiscard]] std::unique_ptr<ast::Stmt> parseStatement();

    /// Parse a single expression from the current token position using
    /// the Pratt (top-down operator-precedence) algorithm.
    /// @param minPrecedence  Minimum binding power for the initial call (0 = all).
    [[nodiscard]] std::unique_ptr<ast::Expr> parseExpression(int minPrecedence = 0);

private:
    // ── Top-level ────────────────────────────────────────────────────────────
    std::unique_ptr<ast::Decl>          parseTopLevelDecl();
    std::unique_ptr<ast::FunctionDecl>  parseFunctionDecl(bool isPub);
    std::unique_ptr<ast::StructDecl>    parseStructDecl(bool isPub);
    std::unique_ptr<ast::EnumDecl>      parseEnumDecl(bool isPub);
    std::unique_ptr<ast::VarDecl>       parseVarDecl();
    std::unique_ptr<ast::TypeAliasDecl> parseTypeAliasDecl(bool isPub);
    std::unique_ptr<ast::ImportDecl>    parseImportDecl();

    // ── Function components ───────────────────────────────────────────────────
    std::vector<std::unique_ptr<ast::ParameterDecl>> parseParamList();
    std::unique_ptr<ast::ParameterDecl>              parseParam();

    // ── Statements ────────────────────────────────────────────────────────────
    std::unique_ptr<ast::Stmt>       parseStmt();
    std::unique_ptr<ast::BlockStmt>  parseBlock();
    std::unique_ptr<ast::IfStmt>     parseIfStmt();
    std::unique_ptr<ast::WhileStmt>  parseWhileStmt();
    std::unique_ptr<ast::VloopStmt>  parseVloopStmt();
    std::unique_ptr<ast::ForStmt>    parseForStmt();
    std::unique_ptr<ast::MatchStmt>  parseMatchStmt();
    std::unique_ptr<ast::ReturnStmt> parseReturnStmt();

    // ── Expressions (Pratt) ───────────────────────────────────────────────────
    std::unique_ptr<ast::Expr> parseExpr(int minPrecedence = 0);
    std::unique_ptr<ast::Expr> parsePrimaryExpr();
    std::unique_ptr<ast::Expr> parsePostfixExpr(std::unique_ptr<ast::Expr> lhs);
    std::unique_ptr<ast::Expr> parseCallArgs(std::unique_ptr<ast::Expr> callee);

    /// Returns the infix precedence for the current token (0 = not an infix op).
    [[nodiscard]] int infixPrecedence() const noexcept;
    [[nodiscard]] ast::BinaryOp tokenToBinaryOp(common::TokenKind k) const noexcept;
    [[nodiscard]] ast::AssignOp tokenToAssignOp(common::TokenKind k) const noexcept;
    [[nodiscard]] bool          isAssignOp(common::TokenKind k) const noexcept;

    // ── Types ────────────────────────────────────────────────────────────────
    std::unique_ptr<ast::TypeNode> parseType();

    // ── Token utilities ──────────────────────────────────────────────────────
    [[nodiscard]] const common::Token& current() const noexcept;
    [[nodiscard]] const common::Token& peek(std::size_t offset = 1) const noexcept;
    common::Token                      advance();
    bool                               check(common::TokenKind k) const noexcept;
    bool                               match(common::TokenKind k);
    common::Token                      expect(common::TokenKind k, std::string_view msg);
    [[nodiscard]] bool                 isAtEnd() const noexcept;

    /// Consume tokens until a synchronisation point (`;` or `}`) is found.
    void synchronize();

    // ── State ────────────────────────────────────────────────────────────────
    common::CompilerContext&   ctx_;
    std::vector<common::Token> tokens_;
    std::size_t                pos_{0};
};

} // namespace vcc::parser
