#pragma once

#include "SourceLocation.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

/// @file Token.h
/// Token kinds and the Token value type produced by the VCC lexer.
///
/// Adding a new keyword
/// ────────────────────
/// 1. Add a `Kw<Name>` enumerator in the Keywords section below.
/// 2. Add an entry to the map returned by Token::keywords().
/// 3. The lexer will automatically classify the identifier.
///
/// Future extensions
/// ─────────────────
///  • Raw string literals (r#"..."#).
///  • Unicode identifier support (XID_Start / XID_Continue).
///  • Byte literals (b'x') and byte-string literals (b"…").

namespace vcc::common {

// ─── Token kind enumeration ──────────────────────────────────────────────────

enum class TokenKind : uint16_t {
    // ── Literals ─────────────────────────────────────────────────────────────
    IntLiteral,     ///< e.g. 42  0xFF  0b1010  0o77
    FloatLiteral,   ///< e.g. 3.14  1.0e-9
    StringLiteral,  ///< "hello\nworld"
    CharLiteral,    ///< 'a'  '\n'

    // ── Identifier ───────────────────────────────────────────────────────────
    Identifier,

    // ── Keywords ─────────────────────────────────────────────────────────────
    KwFn,
    KwFunc,    ///< func — alias for fn
    KwLet,
    KwVar,
    KwConst,
    KwIf,
    KwEf,      ///< ef  — else-if branch
    KwEl,      ///< el  — else branch
    KwElse,
    KwWhile,
    KwFor,
    KwVloop,   ///< vloop — V language loop keyword
    KwIn,
    KwReturn,
    KwImport,
    KwModule,
    KwStruct,
    KwInterface,
    KwEnum,
    KwType,
    KwTrue,
    KwFalse,
    KwNil,
    KwPub,
    KwMut,
    KwSelf,
    KwAs,
    KwBreak,
    KwContinue,
    KwMatch,
    KwArray,   ///< array — array literal constructor

    // ── Arithmetic operators ──────────────────────────────────────────────────
    Plus,      ///< +
    Minus,     ///< -
    Star,      ///< *
    Slash,     ///< /
    Percent,   ///< %

    // ── Comparison operators ──────────────────────────────────────────────────
    EqEq,      ///< ==
    NotEq,     ///< !=
    Lt,        ///< <
    Gt,        ///< >
    LtEq,      ///< <=
    GtEq,      ///< >=

    // ── Logical operators ─────────────────────────────────────────────────────
    AndAnd,    ///< &&
    OrOr,      ///< ||
    Bang,      ///< !

    // ── Bitwise operators ─────────────────────────────────────────────────────
    Amp,       ///< &
    Pipe,      ///< |
    Caret,     ///< ^
    Tilde,     ///< ~
    Shl,       ///< <<
    Shr,       ///< >>

    // ── Assignment operators ──────────────────────────────────────────────────
    Eq,        ///< =
    PlusEq,    ///< +=
    MinusEq,   ///< -=
    StarEq,    ///< *=
    SlashEq,   ///< /=
    PercentEq, ///< %=
    AmpEq,     ///< &=
    PipeEq,    ///< |=
    CaretEq,   ///< ^=

    // ── Punctuation ───────────────────────────────────────────────────────────
    LParen,      ///< (
    RParen,      ///< )
    LBrace,      ///< {
    RBrace,      ///< }
    LBracket,    ///< [
    RBracket,    ///< ]
    Comma,       ///< ,
    Semicolon,   ///< ;
    Colon,       ///< :
    ColonColon,  ///< ::
    Dot,         ///< .
    DotDot,      ///< ..
    DotDotDot,   ///< ...
    Arrow,       ///< ->
    FatArrow,    ///< =>
    At,          ///< @
    Hash,        ///< #
    Question,    ///< ?

    // ── Special ───────────────────────────────────────────────────────────────
    Newline,  ///< Significant newline (used for implicit semicolons, future)
    Eof,      ///< End of input
    Error,    ///< Malformed token; a diagnostic has already been emitted
};

/// Returns a short human-readable name for diagnostics ("'+'", "'fn'", …).
[[nodiscard]] std::string_view tokenKindName(TokenKind kind) noexcept;

/// Returns the uppercase token-type name used by the --tokens output,
/// e.g. "INTEGER", "IDENTIFIER", "LET", "ASSIGN", "PLUS".
[[nodiscard]] std::string_view tokenTypeName(TokenKind kind) noexcept;

/// Convenience alias so user-facing code can write TokenType instead of TokenKind.
using TokenType = TokenKind;

// ─── Token value type ────────────────────────────────────────────────────────

/// A single lexed token.  Cheap to copy; no heap allocation except for the
/// lexeme string (usually short-string-optimised by the standard library).
struct Token {
    TokenKind      kind{TokenKind::Error};
    std::string    lexeme;    ///< Exact source text of this token
    SourceLocation location;

    // ── Predicates ───────────────────────────────────────────────────────────
    [[nodiscard]] bool is   (TokenKind k) const noexcept { return kind == k;  }
    [[nodiscard]] bool isNot(TokenKind k) const noexcept { return kind != k;  }
    [[nodiscard]] bool isEof()            const noexcept { return kind == TokenKind::Eof;   }
    [[nodiscard]] bool isError()          const noexcept { return kind == TokenKind::Error; }

    [[nodiscard]] bool isKeyword()  const noexcept;
    [[nodiscard]] bool isLiteral()  const noexcept;
    [[nodiscard]] bool isOperator() const noexcept;

    // ── Keyword map ──────────────────────────────────────────────────────────
    /// Returns the static keyword → TokenKind lookup table.
    [[nodiscard]] static const std::unordered_map<std::string_view, TokenKind>&
    keywords() noexcept;
};

} // namespace vcc::common
