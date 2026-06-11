#include "vcc/ir/IRInstruction.h"

namespace vcc::ir {

std::string_view opcodeName(Opcode op) noexcept {
    switch (op) {
        case Opcode::Add:        return "add";
        case Opcode::Sub:        return "sub";
        case Opcode::Mul:        return "mul";
        case Opcode::Div:        return "div";
        case Opcode::Mod:        return "mod";
        case Opcode::Neg:        return "neg";
        case Opcode::BitAnd:     return "band";
        case Opcode::BitOr:      return "bor";
        case Opcode::BitXor:     return "bxor";
        case Opcode::BitNot:     return "bnot";
        case Opcode::Shl:        return "shl";
        case Opcode::Shr:        return "shr";
        case Opcode::CmpEq:      return "ceq";
        case Opcode::CmpNe:      return "cne";
        case Opcode::CmpLt:      return "clt";
        case Opcode::CmpGt:      return "cgt";
        case Opcode::CmpLe:      return "cle";
        case Opcode::CmpGe:      return "cge";
        case Opcode::LogAnd:     return "land";
        case Opcode::LogOr:      return "lor";
        case Opcode::LogNot:     return "lnot";
        case Opcode::Alloca:     return "alloca";
        case Opcode::Load:       return "load";
        case Opcode::Store:      return "store";
        case Opcode::Ret:        return "ret";
        case Opcode::Branch:     return "br";
        case Opcode::CondBranch: return "cbr";
        case Opcode::Call:       return "call";
        case Opcode::Param:      return "param";
        case Opcode::Copy:       return "copy";
        case Opcode::Cast:       return "cast";
        case Opcode::Phi:        return "phi";
    }
    return "?";
}

bool IRInstruction::isTerminator() const noexcept {
    return opcode == Opcode::Ret      ||
           opcode == Opcode::Branch   ||
           opcode == Opcode::CondBranch;
}

} // namespace vcc::ir
