#include "vcc/ast/Expressions.h"

namespace vcc::ast {

// ─── binaryOpName ─────────────────────────────────────────────────────────────

std::string_view binaryOpName(BinaryOp op) noexcept {
    switch (op) {
        case BinaryOp::Add:    return "+";
        case BinaryOp::Sub:    return "-";
        case BinaryOp::Mul:    return "*";
        case BinaryOp::Div:    return "/";
        case BinaryOp::Mod:    return "%";
        case BinaryOp::Eq:     return "==";
        case BinaryOp::NotEq:  return "!=";
        case BinaryOp::Lt:     return "<";
        case BinaryOp::Gt:     return ">";
        case BinaryOp::LtEq:   return "<=";
        case BinaryOp::GtEq:   return ">=";
        case BinaryOp::And:    return "&&";
        case BinaryOp::Or:     return "||";
        case BinaryOp::BitAnd: return "&";
        case BinaryOp::BitOr:  return "|";
        case BinaryOp::BitXor: return "^";
        case BinaryOp::Shl:    return "<<";
        case BinaryOp::Shr:    return ">>";
    }
    return "?";
}

// ─── unaryOpName ──────────────────────────────────────────────────────────────

std::string_view unaryOpName(UnaryOp op) noexcept {
    switch (op) {
        case UnaryOp::Neg:    return "-";
        case UnaryOp::Not:    return "!";
        case UnaryOp::BitNot: return "~";
        case UnaryOp::Deref:  return "*";
        case UnaryOp::AddrOf: return "&";
    }
    return "?";
}

} // namespace vcc::ast
