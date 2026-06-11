#include "vcc/lexer/Lexer.h"

#include <cassert>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace vcc::lexer {

using namespace vcc::common;

// ─── Construction ─────────────────────────────────────────────────────────────

Lexer::Lexer(CompilerContext& ctx, FileID fileId)
    : ownedCtx_(nullptr), ctx_(ctx), fileId_(fileId), source_(ctx.sourceText(fileId)) {}

Lexer::Lexer(const std::string& source, const std::string& filename)
    : ownedCtx_(std::make_unique<CompilerContext>()),
      ctx_(*ownedCtx_),
      fileId_(ownedCtx_->addSource(filename, source)),
      source_(ownedCtx_->sourceText(fileId_)) {}

// ─── Public interface ─────────────────────────────────────────────────────────

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.isEof()) break;
    }
    return tokens;
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    if (pos_ >= source_.size()) {
        return makeToken(TokenKind::Eof, pos_);
    }

    const std::size_t start = pos_;
    const char        ch    = advance();

    // ── Identifiers / keywords ────────────────────────────────────────────
    if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
        --pos_; --column_;  // un-consume; lexIdentifierOrKeyword re-reads
        return lexIdentifierOrKeyword();
    }

    // ── Numeric literals ──────────────────────────────────────────────────
    if (std::isdigit(static_cast<unsigned char>(ch))) {
        --pos_; --column_;
        return lexNumber();
    }

    // ── String literals ───────────────────────────────────────────────────
    if (ch == '"') {
        return lexString();
    }

    // ── Character literals ────────────────────────────────────────────────
    if (ch == '\'') {
        return lexChar();
    }

    // ── Multi-character operators / punctuation ───────────────────────────
    switch (ch) {
        case '+': if (match('=')) return makeToken(TokenKind::PlusEq,   start);
                  return makeToken(TokenKind::Plus, start);
        case '-': if (match('>')) return makeToken(TokenKind::Arrow,    start);
                  if (match('=')) return makeToken(TokenKind::MinusEq,  start);
                  return makeToken(TokenKind::Minus, start);
        case '*': if (match('=')) return makeToken(TokenKind::StarEq,   start);
                  return makeToken(TokenKind::Star, start);
        case '/': if (match('=')) return makeToken(TokenKind::SlashEq,  start);
                  return makeToken(TokenKind::Slash, start);
        case '%': if (match('=')) return makeToken(TokenKind::PercentEq,start);
                  return makeToken(TokenKind::Percent, start);
        case '&': if (match('&')) return makeToken(TokenKind::AndAnd,   start);
                  if (match('=')) return makeToken(TokenKind::AmpEq,    start);
                  return makeToken(TokenKind::Amp, start);
        case '|': if (match('|')) return makeToken(TokenKind::OrOr,     start);
                  if (match('=')) return makeToken(TokenKind::PipeEq,   start);
                  return makeToken(TokenKind::Pipe, start);
        case '^': if (match('=')) return makeToken(TokenKind::CaretEq,  start);
                  return makeToken(TokenKind::Caret, start);
        case '~': return makeToken(TokenKind::Tilde, start);
        case '<': if (match('<')) return makeToken(TokenKind::Shl,      start);
                  if (match('=')) return makeToken(TokenKind::LtEq,     start);
                  return makeToken(TokenKind::Lt, start);
        case '>': if (match('>')) return makeToken(TokenKind::Shr,      start);
                  if (match('=')) return makeToken(TokenKind::GtEq,     start);
                  return makeToken(TokenKind::Gt, start);
        case '=': if (match('=')) return makeToken(TokenKind::EqEq,     start);
                  if (match('>')) return makeToken(TokenKind::FatArrow, start);
                  return makeToken(TokenKind::Eq, start);
        case '!': if (match('=')) return makeToken(TokenKind::NotEq,    start);
                  return makeToken(TokenKind::Bang, start);
        case ':': if (match(':')) return makeToken(TokenKind::ColonColon, start);
                  return makeToken(TokenKind::Colon, start);
        case '.': if (match('.')) {
                      if (match('.')) return makeToken(TokenKind::DotDotDot, start);
                      return makeToken(TokenKind::DotDot, start);
                  }
                  return makeToken(TokenKind::Dot, start);
        case '(': return makeToken(TokenKind::LParen,    start);
        case ')': return makeToken(TokenKind::RParen,    start);
        case '{': return makeToken(TokenKind::LBrace,    start);
        case '}': return makeToken(TokenKind::RBrace,    start);
        case '[': return makeToken(TokenKind::LBracket,  start);
        case ']': return makeToken(TokenKind::RBracket,  start);
        case ',': return makeToken(TokenKind::Comma,     start);
        case ';': return makeToken(TokenKind::Semicolon, start);
        case '@': return makeToken(TokenKind::At,        start);
        case '#': return makeToken(TokenKind::Hash,      start);
        case '?': return makeToken(TokenKind::Question,  start);
        default:
            break;
    }

    // Unrecognised character
    std::string msg = "unexpected character '";
    msg += ch;
    msg += '\'';
    return errorToken(std::move(msg));
}

// ─── Character navigation ─────────────────────────────────────────────────────

char Lexer::peek(std::size_t offset) const noexcept {
    const std::size_t idx = pos_ + offset;
    return (idx < source_.size()) ? source_[idx] : '\0';
}

char Lexer::advance() {
    assert(pos_ < source_.size());
    const char ch = source_[pos_++];
    if (ch == '\n') { ++line_; column_ = 1; }
    else            { ++column_; }
    return ch;
}

bool Lexer::match(char expected) {
    if (pos_ >= source_.size() || source_[pos_] != expected) return false;
    advance();
    return true;
}

// ─── Whitespace and comments ──────────────────────────────────────────────────

void Lexer::skipWhitespaceAndComments() {
    while (pos_ < source_.size()) {
        const char ch = peek();
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            advance();
        } else if (ch == '#') {
            skipLineComment();  // V language: # starts a line comment
        } else if (ch == '/' && peek(1) == '/') {
            skipLineComment();
        } else if (ch == '/' && peek(1) == '*') {
            skipBlockComment();
        } else {
            break;
        }
    }
}

void Lexer::skipLineComment() {
    // consume "//"
    advance(); advance();
    while (pos_ < source_.size() && peek() != '\n') advance();
}

void Lexer::skipBlockComment() {
    const SourceLocation startLoc = currentLocation();
    // consume "/*"
    advance(); advance();
    int depth = 1;  // Future: support nested /* … /* … */ … */
    while (pos_ < source_.size() && depth > 0) {
        if (peek() == '*' && peek(1) == '/') {
            advance(); advance();
            --depth;
        } else if (peek() == '/' && peek(1) == '*') {
            advance(); advance();
            ++depth;
        } else {
            advance();
        }
    }
    if (depth > 0) {
        ctx_.diagnostics().error(startLoc, "unterminated block comment");
    }
}

// ─── Identifier / keyword ─────────────────────────────────────────────────────

Token Lexer::lexIdentifierOrKeyword() {
    const std::size_t start = pos_;
    while (pos_ < source_.size() &&
           (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }
    const std::string_view text = source_.substr(start, pos_ - start);

    // Keyword lookup
    const auto& kwMap = Token::keywords();
    if (auto it = kwMap.find(text); it != kwMap.end()) {
        return makeToken(it->second, start);
    }
    return makeToken(TokenKind::Identifier, start);
}

// ─── Numeric literal ──────────────────────────────────────────────────────────

Token Lexer::lexNumber() {
    const std::size_t start = pos_;
    bool isFloat = false;

    // Detect base prefix: 0x, 0b, 0o
    if (peek() == '0') {
        advance();
        if (peek() == 'x' || peek() == 'X') {
            advance();
            while (pos_ < source_.size() && std::isxdigit(static_cast<unsigned char>(peek())))
                advance();
            return makeToken(TokenKind::IntLiteral, start);
        }
        if (peek() == 'b' || peek() == 'B') {
            advance();
            while (peek() == '0' || peek() == '1') advance();
            return makeToken(TokenKind::IntLiteral, start);
        }
        if (peek() == 'o' || peek() == 'O') {
            advance();
            while (peek() >= '0' && peek() <= '7') advance();
            return makeToken(TokenKind::IntLiteral, start);
        }
    }

    // Decimal digits
    while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(peek())))
        advance();

    // Fractional part
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
        isFloat = true;
        advance();  // consume '.'
        while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(peek())))
            advance();
    }

    // Exponent
    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        advance();
        if (peek() == '+' || peek() == '-') advance();
        while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(peek())))
            advance();
    }

    return makeToken(isFloat ? TokenKind::FloatLiteral : TokenKind::IntLiteral, start);
}

// ─── String literal ───────────────────────────────────────────────────────────

Token Lexer::lexString() {
    // Opening '"' already consumed by nextToken.
    const std::size_t start = pos_ - 1;
    const SourceLocation openLoc = currentLocation();

    while (pos_ < source_.size()) {
        const char ch = advance();
        if (ch == '"')  return makeToken(TokenKind::StringLiteral, start);
        if (ch == '\\') {
            // Consume escape sequence — validate in a semantic pass.
            if (pos_ < source_.size()) advance();
        }
        if (ch == '\n') {
            ctx_.diagnostics().error(openLoc, "unterminated string literal");
            return makeToken(TokenKind::Error, start);
        }
    }
    ctx_.diagnostics().error(openLoc, "unterminated string literal");
    return makeToken(TokenKind::Error, start);
}

// ─── Character literal ────────────────────────────────────────────────────────

Token Lexer::lexChar() {
    // Opening '\'' already consumed.
    const std::size_t    start   = pos_ - 1;
    const SourceLocation openLoc = currentLocation();

    if (pos_ >= source_.size()) {
        ctx_.diagnostics().error(openLoc, "unterminated character literal");
        return makeToken(TokenKind::Error, start);
    }

    if (peek() == '\\') advance();  // escape prefix
    if (pos_ < source_.size())  advance();  // the actual char

    if (!match('\'')) {
        ctx_.diagnostics().error(openLoc,
            "character literal may only contain one codepoint");
        // recover: skip to next single-quote or newline
        while (pos_ < source_.size() && peek() != '\'' && peek() != '\n')
            advance();
        if (pos_ < source_.size() && peek() == '\'') advance();
        return makeToken(TokenKind::Error, start);
    }
    return makeToken(TokenKind::CharLiteral, start);
}

// ─── Token factories ──────────────────────────────────────────────────────────

Token Lexer::makeToken(TokenKind kind, std::size_t startPos) const {
    Token tok;
    tok.kind    = kind;
    tok.lexeme  = std::string(source_.substr(startPos, pos_ - startPos));
    tok.location = SourceLocation{fileId_,
                                  line_,
                                  // column of the first character of the token
                                  static_cast<uint32_t>(column_ - (pos_ - startPos))};
    return tok;
}

Token Lexer::errorToken(std::string message) {
    const SourceLocation loc = currentLocation();
    ctx_.diagnostics().error(loc, message);
    Token tok;
    tok.kind     = TokenKind::Error;
    tok.lexeme   = std::string(1, source_[pos_ - 1]);
    tok.location = loc;
    return tok;
}

// ─── Location ────────────────────────────────────────────────────────────────

SourceLocation Lexer::currentLocation() const noexcept {
    return SourceLocation{fileId_, line_, column_};
}

} // namespace vcc::lexer
