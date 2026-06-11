#include "vcc/common/Token.h"

#include <cassert>

namespace vcc::common {

// ─── tokenKindName ────────────────────────────────────────────────────────────

std::string_view tokenKindName(TokenKind kind) noexcept {
    switch (kind) {
        // Literals
        case TokenKind::IntLiteral:    return "<int>";
        case TokenKind::FloatLiteral:  return "<float>";
        case TokenKind::StringLiteral: return "<string>";
        case TokenKind::CharLiteral:   return "<char>";

        // Identifier
        case TokenKind::Identifier:    return "<identifier>";

        // Keywords
        case TokenKind::KwFn:        return "'fn'";
        case TokenKind::KwLet:       return "'let'";
        case TokenKind::KwVar:       return "'var'";
        case TokenKind::KwConst:     return "'const'";
        case TokenKind::KwIf:        return "'if'";
        case TokenKind::KwElse:      return "'else'";
        case TokenKind::KwWhile:     return "'while'";
        case TokenKind::KwFor:       return "'for'";
        case TokenKind::KwIn:        return "'in'";
        case TokenKind::KwReturn:    return "'return'";
        case TokenKind::KwImport:    return "'import'";
        case TokenKind::KwModule:    return "'module'";
        case TokenKind::KwStruct:    return "'struct'";
        case TokenKind::KwInterface: return "'interface'";
        case TokenKind::KwEnum:      return "'enum'";
        case TokenKind::KwType:      return "'type'";
        case TokenKind::KwTrue:      return "'true'";
        case TokenKind::KwFalse:     return "'false'";
        case TokenKind::KwNil:       return "'nil'";
        case TokenKind::KwPub:       return "'pub'";
        case TokenKind::KwMut:       return "'mut'";
        case TokenKind::KwSelf:      return "'self'";
        case TokenKind::KwAs:        return "'as'";
        case TokenKind::KwBreak:     return "'break'";
        case TokenKind::KwContinue:  return "'continue'";
        case TokenKind::KwMatch:     return "'match'";

        // Arithmetic
        case TokenKind::Plus:        return "'+'";
        case TokenKind::Minus:       return "'-'";
        case TokenKind::Star:        return "'*'";
        case TokenKind::Slash:       return "'/'";
        case TokenKind::Percent:     return "'%'";

        // Comparison
        case TokenKind::EqEq:        return "'=='";
        case TokenKind::NotEq:       return "'!='";
        case TokenKind::Lt:          return "'<'";
        case TokenKind::Gt:          return "'>'";
        case TokenKind::LtEq:        return "'<='";
        case TokenKind::GtEq:        return "'>='";

        // Logical
        case TokenKind::AndAnd:      return "'&&'";
        case TokenKind::OrOr:        return "'||'";
        case TokenKind::Bang:        return "'!'";

        // Bitwise
        case TokenKind::Amp:         return "'&'";
        case TokenKind::Pipe:        return "'|'";
        case TokenKind::Caret:       return "'^'";
        case TokenKind::Tilde:       return "'~'";
        case TokenKind::Shl:         return "'<<'";
        case TokenKind::Shr:         return "'>>'";

        // Assignment
        case TokenKind::Eq:          return "'='";
        case TokenKind::PlusEq:      return "'+='";
        case TokenKind::MinusEq:     return "'-='";
        case TokenKind::StarEq:      return "'*='";
        case TokenKind::SlashEq:     return "'/='";
        case TokenKind::PercentEq:   return "'%='";
        case TokenKind::AmpEq:       return "'&='";
        case TokenKind::PipeEq:      return "'|='";
        case TokenKind::CaretEq:     return "'^='";

        // Punctuation
        case TokenKind::LParen:      return "'('";
        case TokenKind::RParen:      return "')'";
        case TokenKind::LBrace:      return "'{'";
        case TokenKind::RBrace:      return "'}'";
        case TokenKind::LBracket:    return "'['";
        case TokenKind::RBracket:    return "']'";
        case TokenKind::Comma:       return "','";
        case TokenKind::Semicolon:   return "';'";
        case TokenKind::Colon:       return "':'";
        case TokenKind::ColonColon:  return "'::'";
        case TokenKind::Dot:         return "'.'";
        case TokenKind::DotDot:      return "'..'";
        case TokenKind::DotDotDot:   return "'...'";
        case TokenKind::Arrow:       return "'->'";
        case TokenKind::FatArrow:    return "'=>'";
        case TokenKind::At:          return "'@'";
        case TokenKind::Hash:        return "'#'";
        case TokenKind::Question:    return "'?'";

        // Special
        case TokenKind::Newline:     return "<newline>";
        case TokenKind::Eof:         return "<eof>";
        case TokenKind::Error:       return "<error>";
    }
    return "<unknown>";
}

// ─── tokenTypeName ────────────────────────────────────────────────────────────

std::string_view tokenTypeName(TokenKind kind) noexcept {
    switch (kind) {
        // Literals
        case TokenKind::IntLiteral:    return "INTEGER";
        case TokenKind::FloatLiteral:  return "FLOAT";
        case TokenKind::StringLiteral: return "STRING";
        case TokenKind::CharLiteral:   return "CHAR";

        // Identifier
        case TokenKind::Identifier:    return "IDENTIFIER";

        // Keywords
        case TokenKind::KwFn:        return "FN";
        case TokenKind::KwLet:       return "LET";
        case TokenKind::KwVar:       return "VAR";
        case TokenKind::KwConst:     return "CONST";
        case TokenKind::KwIf:        return "IF";
        case TokenKind::KwElse:      return "ELSE";
        case TokenKind::KwWhile:     return "WHILE";
        case TokenKind::KwFor:       return "FOR";
        case TokenKind::KwIn:        return "IN";
        case TokenKind::KwReturn:    return "RETURN";
        case TokenKind::KwImport:    return "IMPORT";
        case TokenKind::KwModule:    return "MODULE";
        case TokenKind::KwStruct:    return "STRUCT";
        case TokenKind::KwInterface: return "INTERFACE";
        case TokenKind::KwEnum:      return "ENUM";
        case TokenKind::KwType:      return "TYPE";
        case TokenKind::KwTrue:      return "TRUE";
        case TokenKind::KwFalse:     return "FALSE";
        case TokenKind::KwNil:       return "NIL";
        case TokenKind::KwPub:       return "PUB";
        case TokenKind::KwMut:       return "MUT";
        case TokenKind::KwSelf:      return "SELF";
        case TokenKind::KwAs:        return "AS";
        case TokenKind::KwBreak:     return "BREAK";
        case TokenKind::KwContinue:  return "CONTINUE";
        case TokenKind::KwMatch:     return "MATCH";
        case TokenKind::KwFunc:      return "FUNC";
        case TokenKind::KwEf:        return "EF";
        case TokenKind::KwEl:        return "EL";
        case TokenKind::KwArray:     return "ARRAY";

        // Arithmetic
        case TokenKind::Plus:        return "PLUS";
        case TokenKind::Minus:       return "MINUS";
        case TokenKind::Star:        return "STAR";
        case TokenKind::Slash:       return "SLASH";
        case TokenKind::Percent:     return "PERCENT";

        // Comparison
        case TokenKind::EqEq:        return "EQEQ";
        case TokenKind::NotEq:       return "BANG_EQ";
        case TokenKind::Lt:          return "LT";
        case TokenKind::Gt:          return "GT";
        case TokenKind::LtEq:        return "LT_EQ";
        case TokenKind::GtEq:        return "GT_EQ";

        // Logical
        case TokenKind::AndAnd:      return "AND_AND";
        case TokenKind::OrOr:        return "OR_OR";
        case TokenKind::Bang:        return "BANG";

        // Bitwise
        case TokenKind::Amp:         return "AMP";
        case TokenKind::Pipe:        return "PIPE";
        case TokenKind::Caret:       return "CARET";
        case TokenKind::Tilde:       return "TILDE";
        case TokenKind::Shl:         return "SHL";
        case TokenKind::Shr:         return "SHR";

        // Assignment
        case TokenKind::Eq:          return "ASSIGN";
        case TokenKind::PlusEq:      return "PLUS_EQ";
        case TokenKind::MinusEq:     return "MINUS_EQ";
        case TokenKind::StarEq:      return "STAR_EQ";
        case TokenKind::SlashEq:     return "SLASH_EQ";
        case TokenKind::PercentEq:   return "PERCENT_EQ";
        case TokenKind::AmpEq:       return "AMP_EQ";
        case TokenKind::PipeEq:      return "PIPE_EQ";
        case TokenKind::CaretEq:     return "CARET_EQ";

        // Punctuation
        case TokenKind::LParen:      return "LPAREN";
        case TokenKind::RParen:      return "RPAREN";
        case TokenKind::LBrace:      return "LBRACE";
        case TokenKind::RBrace:      return "RBRACE";
        case TokenKind::LBracket:    return "LBRACKET";
        case TokenKind::RBracket:    return "RBRACKET";
        case TokenKind::Comma:       return "COMMA";
        case TokenKind::Semicolon:   return "SEMICOLON";
        case TokenKind::Colon:       return "COLON";
        case TokenKind::ColonColon:  return "COLON_COLON";
        case TokenKind::Dot:         return "DOT";
        case TokenKind::DotDot:      return "DOT_DOT";
        case TokenKind::DotDotDot:   return "ELLIPSIS";
        case TokenKind::Arrow:       return "ARROW";
        case TokenKind::FatArrow:    return "FAT_ARROW";
        case TokenKind::At:          return "AT";
        case TokenKind::Hash:        return "HASH";
        case TokenKind::Question:    return "QUESTION";

        // Special
        case TokenKind::Newline:     return "NEWLINE";
        case TokenKind::Eof:         return "EOF";
        case TokenKind::Error:       return "ERROR";
    }
    return "UNKNOWN";
}

// ─── Token::keywords ─────────────────────────────────────────────────────────

const std::unordered_map<std::string_view, TokenKind>& Token::keywords() noexcept {
    // Static local — initialised once, thread-safe since C++11.
    static const std::unordered_map<std::string_view, TokenKind> kw {
        {"fn",        TokenKind::KwFn},
        {"func",      TokenKind::KwFunc},
        {"let",       TokenKind::KwLet},
        {"var",       TokenKind::KwVar},
        {"const",     TokenKind::KwConst},
        {"if",        TokenKind::KwIf},
        {"ef",        TokenKind::KwEf},
        {"el",        TokenKind::KwEl},
        {"else",      TokenKind::KwElse},
        {"while",     TokenKind::KwWhile},
        {"for",       TokenKind::KwFor},
        {"vloop",     TokenKind::KwVloop},
        {"in",        TokenKind::KwIn},
        {"return",    TokenKind::KwReturn},
        {"import",    TokenKind::KwImport},
        {"module",    TokenKind::KwModule},
        {"struct",    TokenKind::KwStruct},
        {"interface", TokenKind::KwInterface},
        {"enum",      TokenKind::KwEnum},
        {"type",      TokenKind::KwType},
        {"true",      TokenKind::KwTrue},
        {"false",     TokenKind::KwFalse},
        {"nil",       TokenKind::KwNil},
        {"pub",       TokenKind::KwPub},
        {"mut",       TokenKind::KwMut},
        {"self",      TokenKind::KwSelf},
        {"as",        TokenKind::KwAs},
        {"break",     TokenKind::KwBreak},
        {"continue",  TokenKind::KwContinue},
        {"match",     TokenKind::KwMatch},
        {"array",     TokenKind::KwArray},
    };
    return kw;
}

// ─── Token predicates ─────────────────────────────────────────────────────────

bool Token::isKeyword() const noexcept {
    return kind >= TokenKind::KwFn && kind <= TokenKind::KwArray;
}

bool Token::isLiteral() const noexcept {
    return kind >= TokenKind::IntLiteral && kind <= TokenKind::CharLiteral;
}

bool Token::isOperator() const noexcept {
    return kind >= TokenKind::Plus && kind <= TokenKind::CaretEq;
}

} // namespace vcc::common
