#include "vcc/ir/IRPrinter.h"

#include <iomanip>
#include <sstream>

namespace vcc::ir {

// ─── Constructor ──────────────────────────────────────────────────────────────

IRPrinter::IRPrinter(std::ostream& out) : out_(out) {}

// ─── Top-level entry points ───────────────────────────────────────────────────

void IRPrinter::print(const IRModule& module) {
    out_ << "; VCC IR – module \"" << module.name() << "\"\n\n";
    for (const auto& fn : module.functions()) {
        print(*fn);
        out_ << '\n';
    }
}

void IRPrinter::print(const IRFunction& fn) {
    out_ << "fn " << fn.name() << ":\n";
    const NameMap names = buildNameMap(fn);
    for (const auto& bb : fn.blocks()) {
        printBlock(*bb, names);
    }
}

// ─── NameMap builder ──────────────────────────────────────────────────────────

IRPrinter::NameMap IRPrinter::buildNameMap(const IRFunction& fn) {
    NameMap  names;
    uint32_t tmpCounter = 0;

    for (const auto& bb : fn.blocks()) {
        for (const auto& instr : bb->instructions()) {
            if (instr.dest == NoReg) continue;
            if (names.count(instr.dest)) continue;  // already named

            if (instr.opcode == Opcode::Alloca && !instr.label.empty()) {
                // Named stack slot → use the declared variable name.
                names[instr.dest] = instr.label;
            } else {
                // Anonymous temporary → t1, t2, …
                names[instr.dest] = "t" + std::to_string(++tmpCounter);
            }
        }
    }
    return names;
}

// ─── Block / instruction printers ────────────────────────────────────────────

void IRPrinter::printBlock(const IRBasicBlock& bb, const NameMap& names) {
    out_ << "  " << bb.label() << ":\n";
    for (const auto& instr : bb.instructions()) {
        printInstr(instr, names);
    }
}

void IRPrinter::printInstr(const IRInstruction& instr, const NameMap& names) {
    out_ << "    ";

    // ── Result register ───────────────────────────────────────────────────────
    if (instr.dest != NoReg) {
        out_ << regStr(instr.dest, names) << " = ";
    }

    // ── Opcode ────────────────────────────────────────────────────────────────
    out_ << opcodeName(instr.opcode);

    // ── Operands ──────────────────────────────────────────────────────────────
    switch (instr.opcode) {

        case Opcode::Call:
            // call <name> arg1 arg2 …
            if (!instr.label.empty()) out_ << ' ' << instr.label;
            for (const auto& op : instr.operands)
                out_ << ' ' << operandStr(op, names);
            break;

        case Opcode::Branch:
            // br <target>
            if (!instr.label.empty()) out_ << ' ' << instr.label;
            break;

        case Opcode::CondBranch:
            // cbr <cond> <true-target>,<false-target>
            if (!instr.operands.empty())
                out_ << ' ' << operandStr(instr.operands[0], names);
            if (!instr.label.empty()) out_ << ' ' << instr.label;
            break;

        case Opcode::Alloca:
            // alloca  (name already shown via dest; no need to repeat it)
            break;

        case Opcode::Load: {
            // load <src>  — src may be a named slot or a register
            if (!instr.operands.empty()) {
                out_ << ' ' << operandStr(instr.operands[0], names);
            } else if (!instr.label.empty()) {
                // Symbolic load from a global / undeclared name
                out_ << ' ' << instr.label;
            }
            break;
        }

        case Opcode::Store: {
            // store <value> <dest>
            for (const auto& op : instr.operands)
                out_ << ' ' << operandStr(op, names);
            break;
        }

        case Opcode::Param:
            // param <index>
            if (!instr.operands.empty())
                out_ << ' ' << operandStr(instr.operands[0], names);
            break;

        default:
            // General: operands in order, then optional label annotation
            for (const auto& op : instr.operands)
                out_ << ' ' << operandStr(op, names);
            if (!instr.label.empty())
                out_ << "  ; " << instr.label;
            break;
    }

    out_ << '\n';
}

// ─── Formatting helpers ───────────────────────────────────────────────────────

std::string IRPrinter::regStr(RegID r, const NameMap& names) const {
    if (r == NoReg) return "_";
    auto it = names.find(r);
    if (it != names.end()) return it->second;
    return "%" + std::to_string(r);
}

std::string IRPrinter::operandStr(const Operand& op, const NameMap& names) const {
    switch (op.kind) {
        case Operand::Kind::Reg:
            return regStr(op.reg, names);
        case Operand::Kind::IntImm:
            return std::to_string(op.intVal);
        case Operand::Kind::FloatImm: {
            std::ostringstream oss;
            oss << op.floatVal;
            return oss.str();
        }
        case Operand::Kind::StringRef:
            return '"' + op.strVal + '"';
        case Operand::Kind::BoolImm:
            return op.intVal ? "true" : "false";
    }
    return "?";
}

} // namespace vcc::ir
