#include "vcc/codegen/CodeGen.h"

#include <fstream>
#include <sstream>

namespace vcc::codegen {

using namespace vcc::ir;

// ─── IRTextEmitter ────────────────────────────────────────────────────────────

static std::string operandToString(const Operand& op) {
    switch (op.kind) {
        case Operand::Kind::Reg:       return "%" + std::to_string(op.reg);
        case Operand::Kind::IntImm:    return std::to_string(op.intVal);
        case Operand::Kind::FloatImm:  return std::to_string(op.floatVal);
        case Operand::Kind::StringRef: return '"' + op.strVal + '"';
        case Operand::Kind::BoolImm:   return op.intVal ? "true" : "false";
    }
    return "?";
}

void IRTextEmitter::emit(const IRModule& module, std::ostream& out) const {
    out << "; VCC IR  module " << module.name() << "\n\n";

    for (const auto& fn : module.functions()) {
        out << "fn " << fn->name() << ":\n";
        for (const auto& bb : fn->blocks()) {
            out << "  " << bb->label() << ":\n";
            for (const auto& instr : bb->instructions()) {
                out << "    ";
                if (instr.dest != NoReg) out << "%" << instr.dest << " = ";
                out << opcodeName(instr.opcode);
                for (const auto& op : instr.operands) {
                    out << ' ' << operandToString(op);
                }
                if (!instr.label.empty()) out << " @" << instr.label;
                out << '\n';
            }
        }
        out << '\n';
    }
}

bool IRTextEmitter::generate(const IRModule& module,
                              const std::filesystem::path& outPath) {
    std::ofstream ofs(outPath);
    if (!ofs.is_open()) return false;
    emit(module, ofs);
    return true;
}

} // namespace vcc::codegen
