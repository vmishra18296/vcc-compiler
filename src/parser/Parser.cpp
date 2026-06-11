#include "vcc/parser/Parser.h"

#include <cassert>
#include <stdexcept>

namespace vcc::parser {

using namespace vcc::common;
using namespace vcc::ast;

// ─── Construction ─────────────────────────────────────────────────────────────

Parser::Parser(CompilerContext& ctx, std::vector<Token> tokens)
    : ctx_(ctx), tokens_(std::move(tokens)) {
    assert(!tokens_.empty() && tokens_.back().isEof() &&
           "Token vector must be terminated by Eof");
}

// ─── Public API wrappers ──────────────────────────────────────────────────────

std::unique_ptr<Stmt> Parser::parseStatement() {
    return parseStmt();
}

std::unique_ptr<Expr> Parser::parseExpression(int minPrecedence) {
    return parseExpr(minPrecedence);
}

// ─── Token utilities ──────────────────────────────────────────────────────────

const Token& Parser::current() const noexcept {
    return tokens_[pos_];
}

const Token& Parser::peek(std::size_t offset) const noexcept {
    const std::size_t idx = pos_ + offset;
    return (idx < tokens_.size()) ? tokens_[idx] : tokens_.back();
}

Token Parser::advance() {
    if (!isAtEnd()) ++pos_;
    return tokens_[pos_ - 1];
}

bool Parser::check(TokenKind k) const noexcept {
    return current().is(k);
}

bool Parser::match(TokenKind k) {
    if (check(k)) { advance(); return true; }
    return false;
}

Token Parser::expect(TokenKind k, std::string_view msg) {
    if (check(k)) return advance();
    ctx_.diagnostics().error(current().location,
        std::string(msg) + " (got " + std::string(tokenKindName(current().kind)) + ")");
    // Return a synthetic token so the caller does not crash.
    Token t = current();
    t.kind = k;
    return t;
}

bool Parser::isAtEnd() const noexcept {
    return current().isEof();
}

void Parser::synchronize() {
    // Skip tokens until we reach a natural statement boundary.
    while (!isAtEnd()) {
        if (current().is(TokenKind::Semicolon)) { advance(); return; }
        switch (current().kind) {
            case TokenKind::KwFn:
            case TokenKind::KwFun:
            case TokenKind::KwStruct:
            case TokenKind::KwEnum:
            case TokenKind::KwLet:
            case TokenKind::KwVar:
            case TokenKind::KwConst:
            case TokenKind::KwReturn:
            case TokenKind::KwIf:
            case TokenKind::KwEf:
            case TokenKind::KwMatch:
            case TokenKind::KwWhile:
            case TokenKind::KwVloop:
            case TokenKind::KwFor:
                return;
            default:
                break;
        }
        advance();
    }
}

// ─── Top-level parse ──────────────────────────────────────────────────────────

std::unique_ptr<ModuleDecl> Parser::parse() {
    const SourceLocation start = current().location;

    // Optional module declaration: `module name`
    std::string moduleName = "<anon>";
    if (match(TokenKind::KwModule)) {
        moduleName = expect(TokenKind::Identifier, "expected module name").lexeme;
        match(TokenKind::Semicolon);  // optional trailing semicolon
    }

    auto module = std::make_unique<ModuleDecl>(moduleName);

    // Top-level statements (outside any fun) are collected here and wrapped
    // into a synthetic  fun main() { … }  at end of file.
    std::vector<std::unique_ptr<Stmt>> implicitMain;

    while (!isAtEnd()) {
        // Peek: is this a declaration keyword?
        TokenKind k = current().kind;
        bool isDecl = (k == TokenKind::KwFn   || k == TokenKind::KwFun   ||
                       k == TokenKind::KwPub   || k == TokenKind::KwStruct||
                       k == TokenKind::KwEnum  || k == TokenKind::KwType  ||
                       k == TokenKind::KwImport|| k == TokenKind::KwModule||
                       k == TokenKind::KwLet   || k == TokenKind::KwVar   ||
                       k == TokenKind::KwConst);
        if (isDecl) {
            try {
                auto decl = parseTopLevelDecl();
                if (decl) module->addDecl(std::move(decl));
            } catch (...) {
                ctx_.diagnostics().error(current().location,
                    "parse error; attempting recovery");
                synchronize();
            }
        } else {
            // Bare statement at file scope — fold into implicit main.
            try {
                auto stmt = parseStmt();
                if (stmt) implicitMain.push_back(std::move(stmt));
            } catch (...) {
                ctx_.diagnostics().error(current().location,
                    "parse error; attempting recovery");
                synchronize();
            }
        }
    }

    // Emit synthetic  fun main() { … }  if there were any top-level statements.
    if (!implicitMain.empty()) {
        auto body = std::make_unique<BlockStmt>(std::move(implicitMain));
        auto mainFn = std::make_unique<FunctionDecl>(
            "main",
            std::vector<std::unique_ptr<ParameterDecl>>{},
            /*returnType=*/nullptr,
            std::move(body),
            /*isPub=*/true);
        module->addDecl(std::move(mainFn));
    }

    const SourceLocation end = current().location;
    module->setRange(SourceRange::span(start, end));
    return module;
}

std::unique_ptr<Decl> Parser::parseTopLevelDecl() {
    bool isPub = match(TokenKind::KwPub);

    switch (current().kind) {
        case TokenKind::KwFn:        advance(); return parseFunctionDecl(isPub);
        case TokenKind::KwFun:       advance(); return parseFunctionDecl(isPub);
        case TokenKind::KwStruct:    advance(); return parseStructDecl(isPub);
        case TokenKind::KwEnum:      advance(); return parseEnumDecl(isPub);
        case TokenKind::KwType:      advance(); return parseTypeAliasDecl(isPub);
        case TokenKind::KwImport:    advance(); return parseImportDecl();
        case TokenKind::KwLet:
        case TokenKind::KwVar:
        case TokenKind::KwConst:     return parseVarDecl();
        default:
            ctx_.diagnostics().error(current().location,
                "expected a top-level declaration");
            synchronize();
            return nullptr;
    }
}

// ─── Import ───────────────────────────────────────────────────────────────────

std::unique_ptr<ImportDecl> Parser::parseImportDecl() {
    const SourceLocation loc = current().location;

    std::string path;
    path = expect(TokenKind::Identifier, "expected module path").lexeme;
    while (match(TokenKind::Dot)) {
        path += '.';
        path += expect(TokenKind::Identifier, "expected module path component").lexeme;
    }

    std::optional<std::string> alias;
    if (match(TokenKind::KwAs)) {
        alias = expect(TokenKind::Identifier, "expected alias name").lexeme;
    }
    match(TokenKind::Semicolon);
    return std::make_unique<ImportDecl>(std::move(path), std::move(alias),
                                        SourceRange::at(loc));
}

// ─── Function ─────────────────────────────────────────────────────────────────

std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl(bool isPub) {
    const SourceLocation loc = current().location;
    const std::string name = expect(TokenKind::Identifier, "expected function name").lexeme;

    expect(TokenKind::LParen, "expected '(' after function name");
    auto params = parseParamList();
    expect(TokenKind::RParen, "expected ')' after parameters");

    std::unique_ptr<TypeNode> returnType;
    if (match(TokenKind::Arrow)) {
        returnType = parseType();
    }

    auto body = parseBlock();
    const SourceLocation end = current().location;

    return std::make_unique<FunctionDecl>(
        name, std::move(params), std::move(returnType),
        std::move(body), isPub, SourceRange::span(loc, end));
}

std::vector<std::unique_ptr<ParameterDecl>> Parser::parseParamList() {
    std::vector<std::unique_ptr<ParameterDecl>> params;
    if (check(TokenKind::RParen)) return params;

    do {
        params.push_back(parseParam());
    } while (match(TokenKind::Comma) && !check(TokenKind::RParen));

    return params;
}

std::unique_ptr<ParameterDecl> Parser::parseParam() {
    const SourceLocation loc = current().location;
    bool isMut = match(TokenKind::KwMut);
    const std::string name = expect(TokenKind::Identifier, "expected parameter name").lexeme;
    expect(TokenKind::Colon, "expected ':' after parameter name");
    auto type = parseType();
    return std::make_unique<ParameterDecl>(name, std::move(type), isMut,
                                            SourceRange::at(loc));
}

// ─── Struct ───────────────────────────────────────────────────────────────────

std::unique_ptr<StructDecl> Parser::parseStructDecl(bool isPub) {
    const SourceLocation loc = current().location;
    const std::string name = expect(TokenKind::Identifier, "expected struct name").lexeme;
    expect(TokenKind::LBrace, "expected '{' in struct body");

    std::vector<std::unique_ptr<FieldDecl>> fields;
    while (!check(TokenKind::RBrace) && !isAtEnd()) {
        const SourceLocation floc = current().location;
        bool fieldPub = match(TokenKind::KwPub);
        std::string fname = expect(TokenKind::Identifier, "expected field name").lexeme;
        expect(TokenKind::Colon, "expected ':' after field name");
        auto ftype = parseType();
        match(TokenKind::Comma);
        fields.push_back(std::make_unique<FieldDecl>(
            std::move(fname), std::move(ftype), fieldPub, SourceRange::at(floc)));
    }
    expect(TokenKind::RBrace, "expected '}' after struct fields");
    const SourceLocation end = current().location;
    return std::make_unique<StructDecl>(name, std::move(fields), isPub,
                                         SourceRange::span(loc, end));
}

// ─── Enum ─────────────────────────────────────────────────────────────────────

std::unique_ptr<EnumDecl> Parser::parseEnumDecl(bool isPub) {
    const SourceLocation loc = current().location;
    const std::string name = expect(TokenKind::Identifier, "expected enum name").lexeme;
    expect(TokenKind::LBrace, "expected '{' in enum body");

    std::vector<std::unique_ptr<EnumVariantDecl>> variants;
    while (!check(TokenKind::RBrace) && !isAtEnd()) {
        const SourceLocation vloc = current().location;
        std::string vname = expect(TokenKind::Identifier, "expected variant name").lexeme;
        std::vector<std::unique_ptr<TypeNode>> fields;
        if (match(TokenKind::LParen)) {
            do { fields.push_back(parseType()); } while (match(TokenKind::Comma));
            expect(TokenKind::RParen, "expected ')' after variant fields");
        }
        match(TokenKind::Comma);
        variants.push_back(std::make_unique<EnumVariantDecl>(
            std::move(vname), std::move(fields), SourceRange::at(vloc)));
    }
    expect(TokenKind::RBrace, "expected '}' after enum variants");
    const SourceLocation end = current().location;
    return std::make_unique<EnumDecl>(name, std::move(variants), isPub,
                                       SourceRange::span(loc, end));
}

// ─── Type alias ───────────────────────────────────────────────────────────────

std::unique_ptr<TypeAliasDecl> Parser::parseTypeAliasDecl(bool isPub) {
    const SourceLocation loc = current().location;
    const std::string name = expect(TokenKind::Identifier, "expected type alias name").lexeme;
    expect(TokenKind::Eq, "expected '=' in type alias");
    auto aliased = parseType();
    match(TokenKind::Semicolon);
    return std::make_unique<TypeAliasDecl>(name, std::move(aliased), isPub,
                                            SourceRange::at(loc));
}

// ─── Variable declaration ─────────────────────────────────────────────────────

std::unique_ptr<VarDecl> Parser::parseVarDecl() {
    const SourceLocation loc = current().location;
    VarKind vk = VarKind::Let;
    if      (match(TokenKind::KwLet))   vk = VarKind::Let;
    else if (match(TokenKind::KwVar))   vk = VarKind::Var;
    else if (match(TokenKind::KwConst)) vk = VarKind::Const;

    const std::string name = expect(TokenKind::Identifier, "expected variable name").lexeme;

    std::unique_ptr<TypeNode> type;
    if (match(TokenKind::Colon)) type = parseType();

    std::unique_ptr<Expr> init;
    if (match(TokenKind::Eq)) init = parseExpr();

    match(TokenKind::Semicolon);
    return std::make_unique<VarDecl>(vk, name, std::move(type), std::move(init),
                                      SourceRange::at(loc));
}

// ─── Statements ───────────────────────────────────────────────────────────────

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    const SourceLocation loc = current().location;
    expect(TokenKind::LBrace, "expected '{'");

    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!check(TokenKind::RBrace) && !isAtEnd()) {
        auto s = parseStmt();
        if (s) stmts.push_back(std::move(s));
    }
    expect(TokenKind::RBrace, "expected '}'");
    const SourceLocation end = current().location;
    return std::make_unique<BlockStmt>(std::move(stmts), SourceRange::span(loc, end));
}

std::unique_ptr<Stmt> Parser::parseStmt() {
    const SourceLocation loc = current().location;
    switch (current().kind) {
        case TokenKind::LBrace:    return parseBlock();
        case TokenKind::KwIf:      advance(); return parseIfStmt();
        case TokenKind::KwEf:      advance(); return parseIfStmt();  // bare ef — treated as if
        case TokenKind::KwMatch:   advance(); return parseMatchStmt();
        case TokenKind::KwWhile:   advance(); return parseWhileStmt();
        case TokenKind::KwVloop:   advance(); return parseVloopStmt();
        case TokenKind::KwFor:     advance(); return parseForStmt();
        case TokenKind::KwReturn:  advance(); return parseReturnStmt();
        case TokenKind::KwBreak:
            advance(); match(TokenKind::Semicolon);
            return std::make_unique<BreakStmt>(SourceRange::at(loc));
        case TokenKind::KwContinue:
            advance(); match(TokenKind::Semicolon);
            return std::make_unique<ContinueStmt>(SourceRange::at(loc));
        case TokenKind::KwLet:
        case TokenKind::KwVar:
        case TokenKind::KwConst: {
            auto decl = parseVarDecl();
            return std::make_unique<DeclStmt>(std::move(decl), SourceRange::at(loc));
        }
        default: {
            auto expr = parseExpr();
            match(TokenKind::Semicolon);
            return std::make_unique<ExprStmt>(std::move(expr), SourceRange::at(loc));
        }
    }
}

std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    const SourceLocation loc = current().location;
    // Support both braced and brace-less bodies.
    // Condition may be wrapped in optional parens.
    match(TokenKind::LParen);
    auto cond = parseExpr();
    match(TokenKind::RParen);

    // Body: braced block OR single statement
    std::unique_ptr<Stmt> then;
    if (check(TokenKind::LBrace)) {
        then = parseBlock();
    } else {
        then = parseStmt();
    }

    std::unique_ptr<Stmt> els;
    // Accept ef (else-if) and el (else) as well as legacy else/else if
    if (check(TokenKind::KwEf) || (check(TokenKind::KwElse) && peek(1).kind == TokenKind::KwIf)) {
        advance(); // consume ef / else
        if (check(TokenKind::KwIf)) advance(); // consume optional if after else
        els = parseIfStmt();
    } else if (check(TokenKind::KwEl) || check(TokenKind::KwElse)) {
        advance(); // consume el / else
        if (check(TokenKind::LBrace)) {
            els = parseBlock();
        } else {
            els = parseStmt();
        }
    }
    return std::make_unique<IfStmt>(std::move(cond), std::move(then),
                                     std::move(els), SourceRange::at(loc));
}

std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
    const SourceLocation loc = current().location;
    auto cond = parseExpr();
    auto body = parseBlock();
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body),
                                        SourceRange::at(loc));
}

std::unique_ptr<VloopStmt> Parser::parseVloopStmt() {
    const SourceLocation loc = current().location;
    expect(TokenKind::LParen, "expected '(' after 'vloop'");
    auto cond = parseExpr();
    expect(TokenKind::RParen, "expected ')' after vloop condition");
    auto body = parseBlock();
    return std::make_unique<VloopStmt>(std::move(cond), std::move(body),
                                       SourceRange::at(loc));
}

std::unique_ptr<MatchStmt> Parser::parseMatchStmt() {
    // match subject { pattern => stmt  ...  _ => stmt }
    const SourceLocation loc = current().location;

    // Optional parens around subject: match(x) { } or match x { }
    match(TokenKind::LParen);
    auto subject = parseExpr();
    match(TokenKind::RParen);

    expect(TokenKind::LBrace, "expected '{' after match subject");
    std::vector<MatchArm> arms;

    while (!check(TokenKind::RBrace) && !isAtEnd()) {
        MatchArm arm;
        if (check(TokenKind::Identifier) && current().lexeme == "_") {
            advance();                           // wildcard '_'
            arm.pattern = nullptr;
        } else {
            arm.pattern = parseExpr();
        }
        expect(TokenKind::FatArrow, "expected '=>' in match arm");

        if (check(TokenKind::LBrace)) {
            arm.body = parseBlock();
        } else {
            arm.body = parseStmt();
        }
        match(TokenKind::Comma);  // optional trailing comma
        arms.push_back(std::move(arm));
    }
    expect(TokenKind::RBrace, "expected '}' after match arms");
    return std::make_unique<MatchStmt>(std::move(subject), std::move(arms),
                                       SourceRange::at(loc));
}

std::unique_ptr<ForStmt> Parser::parseForStmt() {
    const SourceLocation loc = current().location;
    std::string var = expect(TokenKind::Identifier, "expected loop variable").lexeme;
    expect(TokenKind::KwIn, "expected 'in' in for loop");
    auto iter = parseExpr();
    auto body = parseBlock();
    return std::make_unique<ForStmt>(std::move(var), std::move(iter),
                                      std::move(body), SourceRange::at(loc));
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    const SourceLocation loc = current().location;
    std::unique_ptr<Expr> val;
    if (!check(TokenKind::Semicolon) && !check(TokenKind::RBrace) && !isAtEnd()) {
        val = parseExpr();
    }
    match(TokenKind::Semicolon);
    return std::make_unique<ReturnStmt>(std::move(val), SourceRange::at(loc));
}

// ─── Expressions (Pratt) ──────────────────────────────────────────────────────

int Parser::infixPrecedence() const noexcept {
    // Standard C-like precedence levels (higher = tighter binding)
    switch (current().kind) {
        case TokenKind::OrOr:      return 1;
        case TokenKind::AndAnd:    return 2;
        case TokenKind::Pipe:      return 3;
        case TokenKind::Caret:     return 4;
        case TokenKind::Amp:       return 5;
        case TokenKind::EqEq:
        case TokenKind::NotEq:     return 6;
        case TokenKind::Lt:
        case TokenKind::Gt:
        case TokenKind::LtEq:
        case TokenKind::GtEq:      return 7;
        case TokenKind::Shl:
        case TokenKind::Shr:       return 8;
        case TokenKind::Plus:
        case TokenKind::Minus:     return 9;
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:   return 10;
        // Assignment ops have lower precedence; handled separately in parseExpr
        default:                   return 0;
    }
}

BinaryOp Parser::tokenToBinaryOp(TokenKind k) const noexcept {
    switch (k) {
        case TokenKind::Plus:    return BinaryOp::Add;
        case TokenKind::Minus:   return BinaryOp::Sub;
        case TokenKind::Star:    return BinaryOp::Mul;
        case TokenKind::Slash:   return BinaryOp::Div;
        case TokenKind::Percent: return BinaryOp::Mod;
        case TokenKind::EqEq:    return BinaryOp::Eq;
        case TokenKind::NotEq:   return BinaryOp::NotEq;
        case TokenKind::Lt:      return BinaryOp::Lt;
        case TokenKind::Gt:      return BinaryOp::Gt;
        case TokenKind::LtEq:    return BinaryOp::LtEq;
        case TokenKind::GtEq:    return BinaryOp::GtEq;
        case TokenKind::AndAnd:  return BinaryOp::And;
        case TokenKind::OrOr:    return BinaryOp::Or;
        case TokenKind::Amp:     return BinaryOp::BitAnd;
        case TokenKind::Pipe:    return BinaryOp::BitOr;
        case TokenKind::Caret:   return BinaryOp::BitXor;
        case TokenKind::Shl:     return BinaryOp::Shl;
        case TokenKind::Shr:     return BinaryOp::Shr;
        default:                 return BinaryOp::Add; // unreachable
    }
}

AssignOp Parser::tokenToAssignOp(TokenKind k) const noexcept {
    switch (k) {
        case TokenKind::Eq:        return AssignOp::Assign;
        case TokenKind::PlusEq:    return AssignOp::AddAssign;
        case TokenKind::MinusEq:   return AssignOp::SubAssign;
        case TokenKind::StarEq:    return AssignOp::MulAssign;
        case TokenKind::SlashEq:   return AssignOp::DivAssign;
        case TokenKind::PercentEq: return AssignOp::ModAssign;
        case TokenKind::AmpEq:     return AssignOp::BitAndAssign;
        case TokenKind::PipeEq:    return AssignOp::BitOrAssign;
        case TokenKind::CaretEq:   return AssignOp::BitXorAssign;
        default:                   return AssignOp::Assign;
    }
}

bool Parser::isAssignOp(TokenKind k) const noexcept {
    switch (k) {
        case TokenKind::Eq:
        case TokenKind::PlusEq:
        case TokenKind::MinusEq:
        case TokenKind::StarEq:
        case TokenKind::SlashEq:
        case TokenKind::PercentEq:
        case TokenKind::AmpEq:
        case TokenKind::PipeEq:
        case TokenKind::CaretEq:
            return true;
        default:
            return false;
    }
}

std::unique_ptr<Expr> Parser::parseExpr(int minPrec) {
    auto lhs = parsePrimaryExpr();
    lhs = parsePostfixExpr(std::move(lhs));

    // Handle assignment (right-associative, lowest precedence)
    if (isAssignOp(current().kind)) {
        const SourceLocation loc = current().location;
        AssignOp op = tokenToAssignOp(current().kind);
        advance();
        auto rhs = parseExpr(0);
        return std::make_unique<AssignExpr>(op, std::move(lhs), std::move(rhs),
                                             SourceRange::at(loc));
    }

    // Binary infix operators
    while (true) {
        int prec = infixPrecedence();
        if (prec <= minPrec) break;
        const SourceLocation loc = current().location;
        BinaryOp op = tokenToBinaryOp(current().kind);
        advance();
        auto rhs = parseExpr(prec);  // left-associative: prec (not prec-1)
        lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs),
                                            SourceRange::at(loc));
        lhs = parsePostfixExpr(std::move(lhs));
    }

    return lhs;
}

std::unique_ptr<Expr> Parser::parsePrimaryExpr() {
    const SourceLocation loc = current().location;

    // Prefix unary
    switch (current().kind) {
        case TokenKind::Minus: {
            advance();
            auto operand = parsePrimaryExpr();
            return std::make_unique<UnaryExpr>(UnaryOp::Neg, std::move(operand),
                                               SourceRange::at(loc));
        }
        case TokenKind::Bang: {
            advance();
            auto operand = parsePrimaryExpr();
            return std::make_unique<UnaryExpr>(UnaryOp::Not, std::move(operand),
                                               SourceRange::at(loc));
        }
        case TokenKind::Tilde: {
            advance();
            auto operand = parsePrimaryExpr();
            return std::make_unique<UnaryExpr>(UnaryOp::BitNot, std::move(operand),
                                               SourceRange::at(loc));
        }
        case TokenKind::Star: {
            advance();
            auto operand = parsePrimaryExpr();
            return std::make_unique<UnaryExpr>(UnaryOp::Deref, std::move(operand),
                                               SourceRange::at(loc));
        }
        case TokenKind::Amp: {
            advance();
            auto operand = parsePrimaryExpr();
            return std::make_unique<UnaryExpr>(UnaryOp::AddrOf, std::move(operand),
                                               SourceRange::at(loc));
        }
        default: break;
    }

    // Grouping
    if (match(TokenKind::LParen)) {
        auto expr = parseExpr();
        expect(TokenKind::RParen, "expected ')' after expression");
        return expr;
    }

    // Literals
    switch (current().kind) {
        case TokenKind::IntLiteral: {
            int64_t val = std::stoll(current().lexeme, nullptr, 0);
            advance();
            return std::make_unique<IntLiteralExpr>(val, SourceRange::at(loc));
        }
        case TokenKind::FloatLiteral: {
            double val = std::stod(current().lexeme);
            advance();
            return std::make_unique<FloatLiteralExpr>(val, SourceRange::at(loc));
        }
        case TokenKind::StringLiteral: {
            std::string val = current().lexeme;
            // Strip surrounding quotes; escape processing deferred to sema.
            if (val.size() >= 2) val = val.substr(1, val.size() - 2);
            advance();
            return std::make_unique<StringLiteralExpr>(std::move(val), SourceRange::at(loc));
        }
        case TokenKind::CharLiteral: {
            // Simple ASCII extraction; Unicode deferred.
            char32_t val = (current().lexeme.size() > 1) ?
                static_cast<char32_t>(current().lexeme[1]) : 0;
            advance();
            return std::make_unique<CharLiteralExpr>(val, SourceRange::at(loc));
        }
        case TokenKind::KwTrue:
            advance();
            return std::make_unique<BoolLiteralExpr>(true, SourceRange::at(loc));
        case TokenKind::KwFalse:
            advance();
            return std::make_unique<BoolLiteralExpr>(false, SourceRange::at(loc));
        case TokenKind::KwNil:
            advance();
            return std::make_unique<NilLiteralExpr>(SourceRange::at(loc));
        case TokenKind::KwArray: {
            // array[elem, elem, ...]
            advance(); // consume 'array'
            expect(TokenKind::LBracket, "expected '[' after 'array'");
            std::vector<std::unique_ptr<Expr>> elems;
            if (!check(TokenKind::RBracket)) {
                do { elems.push_back(parseExpr()); } while (match(TokenKind::Comma));
            }
            expect(TokenKind::RBracket, "expected ']' after array elements");
            return std::make_unique<ArrayLiteralExpr>(std::move(elems), SourceRange::at(loc));
        }
        default: break;
    }

    // Identifier
    if (check(TokenKind::Identifier)) {
        std::string name = current().lexeme;
        advance();
        return std::make_unique<IdentExpr>(std::move(name), SourceRange::at(loc));
    }

    ctx_.diagnostics().error(loc, "expected an expression (got " +
        std::string(tokenKindName(current().kind)) + ")");
    // Return a sentinel rather than nullptr to avoid null-ptr cascades.
    auto sentinel = std::make_unique<NilLiteralExpr>(SourceRange::at(loc));
    if (!isAtEnd()) advance();  // skip the bad token
    return sentinel;
}

std::unique_ptr<Expr> Parser::parsePostfixExpr(std::unique_ptr<Expr> lhs) {
    while (true) {
        const SourceLocation loc = current().location;
        if (match(TokenKind::LParen)) {
            lhs = parseCallArgs(std::move(lhs));
        } else if (match(TokenKind::LBracket)) {
            auto index = parseExpr();
            expect(TokenKind::RBracket, "expected ']'");
            lhs = std::make_unique<IndexExpr>(std::move(lhs), std::move(index),
                                               SourceRange::at(loc));
        } else if (match(TokenKind::Dot)) {
            std::string member = expect(TokenKind::Identifier, "expected field name").lexeme;
            lhs = std::make_unique<MemberExpr>(std::move(lhs), std::move(member),
                                                SourceRange::at(loc));
        } else if (match(TokenKind::KwAs)) {
            auto targetType = parseType();
            lhs = std::make_unique<CastExpr>(std::move(lhs), std::move(targetType),
                                              SourceRange::at(loc));
        } else {
            break;
        }
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseCallArgs(std::unique_ptr<Expr> callee) {
    const SourceLocation loc = current().location;
    std::vector<std::unique_ptr<Expr>> args;
    if (!check(TokenKind::RParen)) {
        do { args.push_back(parseExpr()); } while (match(TokenKind::Comma));
    }
    expect(TokenKind::RParen, "expected ')' after arguments");
    return std::make_unique<CallExpr>(std::move(callee), std::move(args),
                                       SourceRange::at(loc));
}

// ─── Types ────────────────────────────────────────────────────────────────────

std::unique_ptr<TypeNode> Parser::parseType() {
    const SourceLocation loc = current().location;

    // Pointer type: *T or *mut T
    if (match(TokenKind::Star)) {
        bool isMut = match(TokenKind::KwMut);
        auto inner = parseType();
        return std::make_unique<PointerTypeNode>(std::move(inner), isMut,
                                                  SourceRange::at(loc));
    }

    // Slice / array type: [T] or [T; N]
    if (match(TokenKind::LBracket)) {
        auto elem = parseType();
        if (match(TokenKind::Semicolon)) {
            auto size = parseExpr();
            expect(TokenKind::RBracket, "expected ']'");
            return std::make_unique<ArrayTypeNode>(std::move(elem), std::move(size),
                                                    SourceRange::at(loc));
        }
        expect(TokenKind::RBracket, "expected ']'");
        return std::make_unique<SliceTypeNode>(std::move(elem), SourceRange::at(loc));
    }

    // Function type: fn/func (T1, T2) -> R
    if (match(TokenKind::KwFn) || match(TokenKind::KwFun)) {
        expect(TokenKind::LParen, "expected '(' in function type");
        std::vector<std::unique_ptr<TypeNode>> params;
        if (!check(TokenKind::RParen)) {
            do { params.push_back(parseType()); } while (match(TokenKind::Comma));
        }
        expect(TokenKind::RParen, "expected ')' in function type");
        std::unique_ptr<TypeNode> ret;
        if (match(TokenKind::Arrow)) ret = parseType();
        return std::make_unique<FunctionTypeNode>(std::move(params), std::move(ret),
                                                   SourceRange::at(loc));
    }

    // Named type (simple identifier or qualified path)
    std::string name = expect(TokenKind::Identifier, "expected type name").lexeme;
    while (match(TokenKind::ColonColon)) {
        name += "::";
        name += expect(TokenKind::Identifier, "expected type name component").lexeme;
    }
    return std::make_unique<NamedTypeNode>(std::move(name), SourceRange::at(loc));
}

} // namespace vcc::parser
