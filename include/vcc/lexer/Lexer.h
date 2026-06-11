#pragma once

#include "vcc/common/CompilerContext.h"
#include "vcc/common/Token.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

/// @file Lexer.h
/// VCC tokeniser – converts raw V source text into a flat token stream.
///
/// Design
/// ──────
///  • Single-pass, linear-time over the input.
///  • Tracks line and column for every character consumed.
///  • Emits diagnostics for malformed tokens but continues scanning so that
///    the parser can collect multiple errors in one run.
///  • Keywords are recognised via a hash-map lookup after scanning an
///    identifier – no special lexer state required.
///
/// Future extensions
/// ─────────────────
///  • Unicode identifier support (XID_Start / XID_Continue via ICU or libucd).
///  • Raw string literals:  r#"…"#
///  • Byte/byte-string literals.
///  • Implicit semicolons (NEWLINE token promotion on specific trailing tokens).
///  • Incremental re-lexing for LSP use.

namespace vcc::lexer {

class Lexer {
public:
    /// Construct a lexer bound to @p ctx and the source registered as @p fileId.
    Lexer(common::CompilerContext& ctx, common::FileID fileId);

    /// Convenience constructor: takes source text directly.
    /// Creates an internal CompilerContext; use diagnostics() to inspect errors.
    explicit Lexer(const std::string& source,
                   const std::string& filename = "<string>");

    /// Lex and return the next token from the input.
    /// After EOF is reached every subsequent call returns an Eof token.
    [[nodiscard]] common::Token nextToken();

    /// Convenience: lex the entire input and return all tokens including Eof.
    [[nodiscard]] std::vector<common::Token> tokenize();

    /// Access diagnostics emitted during lexing (useful with the string ctor).
    [[nodiscard]] const common::DiagnosticsEngine& diagnostics() const noexcept {
        return ctx_.diagnostics();
    }

private:
    // ── Character navigation ──────────────────────────────────────────────
    [[nodiscard]] char peek(std::size_t offset = 0) const noexcept;
    char               advance();
    bool               match(char expected);

    // ── Whitespace / comments ─────────────────────────────────────────────
    void skipWhitespaceAndComments();
    void skipLineComment();
    void skipBlockComment();

    // ── Sub-lexers ────────────────────────────────────────────────────────
    common::Token lexIdentifierOrKeyword();
    common::Token lexNumber();
    common::Token lexString();
    common::Token lexChar();

    // ── Token factories ───────────────────────────────────────────────────
    common::Token makeToken(common::TokenKind kind, std::size_t startPos) const;
    common::Token errorToken(std::string message);

    // ── Location helpers ──────────────────────────────────────────────────
    [[nodiscard]] common::SourceLocation currentLocation() const noexcept;

    // ── State ─────────────────────────────────────────────────────────────
    std::unique_ptr<common::CompilerContext> ownedCtx_; ///< Non-null only for string ctor
    common::CompilerContext& ctx_;
    common::FileID           fileId_;
    std::string_view         source_;

    std::size_t pos_{0};       ///< Current byte offset into source_
    uint32_t    line_{1};      ///< Current 1-based line number
    uint32_t    column_{1};    ///< Current 1-based column number
};

} // namespace vcc::lexer
