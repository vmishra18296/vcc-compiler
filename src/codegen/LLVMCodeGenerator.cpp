#ifdef VCC_ENABLE_LLVM

#include "vcc/codegen/LLVMCodeGenerator.h"
#include "vcc/ir/IRInstruction.h"
#include "vcc/ir/IRModule.h"

// ── LLVM headers ──────────────────────────────────────────────────────────────
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vcc::codegen {

using namespace llvm;

// ─── Per-function lowering state ──────────────────────────────────────────────

namespace {

/// Lowers one VCC IRFunction into an llvm::Function inside @p mod.
///
/// Register model
/// ──────────────
/// Every VCC RegID maps to one of:
///   • AllocaInst*  — for Alloca results (the slot's address, type ptr)
///   • Value*       — for everything else (an i64 value)
///
/// Type policy (Phase 1)
/// ─────────────────────
/// All arithmetic / memory values are i64.  Compare / logical results are
/// widened back to i64 via zext.  Branch conditions are narrowed to i1.
struct FunctionLowering {
    LLVMContext&              ctx;
    Module&                   mod;
    const ir::IRFunction&     irFn;
    Function*                 fn{nullptr};
    IRBuilder<>               builder;

    // RegID → LLVM Value*  (AllocaInst* for stack slots, i64 for values)
    std::unordered_map<ir::RegID, Value*> regs;

    // VCC block label → llvm::BasicBlock*
    std::unordered_map<std::string, BasicBlock*> blocks;

    explicit FunctionLowering(LLVMContext& c, Module& m,
                               const ir::IRFunction& f)
        : ctx(c), mod(m), irFn(f), builder(c) {}

    // ── Type helpers ──────────────────────────────────────────────────────────

    Type* i64Ty()  const { return Type::getInt64Ty(ctx);  }
    Type* i1Ty()   const { return Type::getInt1Ty(ctx);   }
    Type* voidTy() const { return Type::getVoidTy(ctx);   }

    Value* zeroI64() const {
        return ConstantInt::get(Type::getInt64Ty(ctx), 0, /*isSigned=*/true);
    }

    /// Widen an i1 value to i64 via zext (no-op if already i64).
    Value* widenToI64(Value* v) {
        if (v->getType()->isIntegerTy(1))
            return builder.CreateZExt(v, i64Ty(), "zext");
        return v;
    }

    /// Narrow a value to i1 for use as a branch condition (icmp ne val, 0).
    Value* toI1(Value* v) {
        if (v->getType()->isIntegerTy(1)) return v;
        return builder.CreateICmpNE(v, zeroI64(), "tobool");
    }

    // ── Signature inference ───────────────────────────────────────────────────

    /// Count `Param` instructions in the entry block.
    uint32_t countParams() const {
        if (irFn.blocks().empty()) return 0;
        const auto* entry = irFn.blocks().front().get();
        if (!entry) return 0;
        uint32_t n = 0;
        for (const auto& instr : entry->instructions())
            if (instr.opcode == ir::Opcode::Param) ++n;
        return n;
    }

    /// True iff any Ret instruction in the function carries a value operand.
    bool hasReturnValue() const {
        for (const auto& bb : irFn.blocks())
            for (const auto& instr : bb->instructions())
                if (instr.opcode == ir::Opcode::Ret && !instr.operands.empty())
                    return true;
        return false;
    }

    // ── Create llvm::Function ─────────────────────────────────────────────────

    void createFunction() {
        const uint32_t nParams = countParams();
        const bool     hasRet  = hasReturnValue();

        std::vector<Type*> paramTypes(nParams, i64Ty());
        Type* retType = hasRet ? i64Ty() : voidTy();

        FunctionType* fnType =
            FunctionType::get(retType, paramTypes, /*isVarArg=*/false);
        fn = Function::Create(fnType, GlobalValue::ExternalLinkage,
                               irFn.name(), mod);

        // Give arguments readable names for the .ll output.
        uint32_t argIdx = 0;
        for (auto& arg : fn->args())
            arg.setName("arg" + std::to_string(argIdx++));
    }

    // ── Pre-create all BasicBlocks ────────────────────────────────────────────

    void createBlocks() {
        for (const auto& bb : irFn.blocks()) {
            BasicBlock* llvmBB =
                BasicBlock::Create(ctx, bb->label(), fn);
            blocks[bb->label()] = llvmBB;
        }
    }

    // ── Operand resolution ────────────────────────────────────────────────────

    /// Resolve a VCC Operand to an llvm::Value*.
    Value* resolve(const ir::Operand& op) {
        switch (op.kind) {
            case ir::Operand::Kind::Reg: {
                auto it = regs.find(op.reg);
                if (it != regs.end()) return it->second;
                // Undefined reference — return i64 undef.
                return UndefValue::get(i64Ty());
            }
            case ir::Operand::Kind::IntImm:
                return ConstantInt::get(i64Ty(), op.intVal, /*isSigned=*/true);
            case ir::Operand::Kind::FloatImm:
                return ConstantFP::get(Type::getDoubleTy(ctx), op.floatVal);
            case ir::Operand::Kind::BoolImm:
                return ConstantInt::get(i64Ty(), op.intVal != 0 ? 1 : 0);
            case ir::Operand::Kind::StringRef:
                // Phase 1: string not yet lowered — produce i64 0 as placeholder.
                return ConstantInt::get(i64Ty(), 0);
        }
        return UndefValue::get(i64Ty());
    }

    /// Resolve two operands for binary instructions.
    std::pair<Value*, Value*> two(const ir::IRInstruction& instr) {
        Value* lhs = instr.operands.size() > 0
                         ? resolve(instr.operands[0])
                         : UndefValue::get(i64Ty());
        Value* rhs = instr.operands.size() > 1
                         ? resolve(instr.operands[1])
                         : UndefValue::get(i64Ty());
        return {lhs, rhs};
    }

    /// Resolve the single operand for unary instructions.
    Value* one(const ir::IRInstruction& instr) {
        return instr.operands.empty() ? UndefValue::get(i64Ty())
                                      : resolve(instr.operands[0]);
    }

    // ── Emit one VCC instruction ──────────────────────────────────────────────

    void emit(const ir::IRInstruction& instr) {
        switch (instr.opcode) {

        // ── Memory ────────────────────────────────────────────────────────────

        case ir::Opcode::Alloca: {
            // `name = alloca`  →  %name = alloca i64
            AllocaInst* slot = builder.CreateAlloca(
                i64Ty(), nullptr,
                instr.label.empty() ? "slot" : instr.label);
            regs[instr.dest] = slot;
            break;
        }

        case ir::Opcode::Param: {
            // `tN = param K`  →  map to the K-th LLVM function argument
            if (!instr.operands.empty()) {
                const auto idx = static_cast<unsigned>(instr.operands[0].intVal);
                auto argIt = fn->arg_begin();
                std::advance(argIt, idx);
                regs[instr.dest] = &*argIt;
            }
            break;
        }

        case ir::Opcode::Load: {
            // `tN = load ptrReg`  or  `tN = load symbolName` (symbolic)
            Value* result = nullptr;
            if (!instr.operands.empty()) {
                Value* ptr = resolve(instr.operands[0]);
                result = builder.CreateLoad(
                    i64Ty(), ptr,
                    instr.label.empty() ? "load" : instr.label);
            } else {
                // Symbolic / global load — produce undef in Phase 1.
                result = UndefValue::get(i64Ty());
            }
            if (instr.dest != ir::NoReg) regs[instr.dest] = result;
            break;
        }

        case ir::Opcode::Store: {
            // `store valueReg ptrReg`
            if (instr.operands.size() >= 2) {
                Value* val = resolve(instr.operands[0]);
                Value* ptr = resolve(instr.operands[1]);
                builder.CreateStore(val, ptr);
            }
            break;
        }

        case ir::Opcode::Copy: {
            // `tN = copy <imm>`  — literal value
            if (!instr.operands.empty() && instr.dest != ir::NoReg)
                regs[instr.dest] = resolve(instr.operands[0]);
            break;
        }

        // ── Arithmetic ────────────────────────────────────────────────────────

        case ir::Opcode::Add: {
            auto [l, r] = two(instr);
            regs[instr.dest] = builder.CreateAdd(l, r, "add");
            break;
        }
        case ir::Opcode::Sub: {
            auto [l, r] = two(instr);
            regs[instr.dest] = builder.CreateSub(l, r, "sub");
            break;
        }
        case ir::Opcode::Mul: {
            auto [l, r] = two(instr);
            regs[instr.dest] = builder.CreateMul(l, r, "mul");
            break;
        }
        case ir::Opcode::Div: {
            auto [l, r] = two(instr);
            regs[instr.dest] = builder.CreateSDiv(l, r, "div");
            break;
        }
        case ir::Opcode::Mod: {
            auto [l, r] = two(instr);
            regs[instr.dest] = builder.CreateSRem(l, r, "mod");
            break;
        }
        case ir::Opcode::Neg: {
            regs[instr.dest] = builder.CreateNeg(one(instr), "neg");
            break;
        }

        // ── Bitwise ───────────────────────────────────────────────────────────

        case ir::Opcode::BitAnd: {
            auto [l, r] = two(instr);
            regs[instr.dest] = builder.CreateAnd(l, r, "band");
            break;
        }
        case ir::Opcode::BitOr: {
            auto [l, r] = two(instr);
            regs[instr.dest] = builder.CreateOr(l, r, "bor");
            break;
        }
        case ir::Opcode::BitXor: {
            auto [l, r] = two(instr);
            regs[instr.dest] = builder.CreateXor(l, r, "bxor");
            break;
        }
        case ir::Opcode::BitNot: {
            regs[instr.dest] = builder.CreateNot(one(instr), "bnot");
            break;
        }
        case ir::Opcode::Shl: {
            auto [l, r] = two(instr);
            regs[instr.dest] = builder.CreateShl(l, r, "shl");
            break;
        }
        case ir::Opcode::Shr: {
            auto [l, r] = two(instr);
            // Arithmetic (signed) right shift.
            regs[instr.dest] = builder.CreateAShr(l, r, "ashr");
            break;
        }

        // ── Comparisons (→ i64 via zext) ──────────────────────────────────────

        case ir::Opcode::CmpEq: {
            auto [l, r] = two(instr);
            regs[instr.dest] = widenToI64(builder.CreateICmpEQ(l, r, "eq"));
            break;
        }
        case ir::Opcode::CmpNe: {
            auto [l, r] = two(instr);
            regs[instr.dest] = widenToI64(builder.CreateICmpNE(l, r, "ne"));
            break;
        }
        case ir::Opcode::CmpLt: {
            auto [l, r] = two(instr);
            regs[instr.dest] = widenToI64(builder.CreateICmpSLT(l, r, "lt"));
            break;
        }
        case ir::Opcode::CmpGt: {
            auto [l, r] = two(instr);
            regs[instr.dest] = widenToI64(builder.CreateICmpSGT(l, r, "gt"));
            break;
        }
        case ir::Opcode::CmpLe: {
            auto [l, r] = two(instr);
            regs[instr.dest] = widenToI64(builder.CreateICmpSLE(l, r, "le"));
            break;
        }
        case ir::Opcode::CmpGe: {
            auto [l, r] = two(instr);
            regs[instr.dest] = widenToI64(builder.CreateICmpSGE(l, r, "ge"));
            break;
        }

        // ── Logical (→ i64 via zext) ──────────────────────────────────────────

        case ir::Opcode::LogAnd: {
            auto [l, r] = two(instr);
            regs[instr.dest] =
                widenToI64(builder.CreateAnd(toI1(l), toI1(r), "land"));
            break;
        }
        case ir::Opcode::LogOr: {
            auto [l, r] = two(instr);
            regs[instr.dest] =
                widenToI64(builder.CreateOr(toI1(l), toI1(r), "lor"));
            break;
        }
        case ir::Opcode::LogNot: {
            regs[instr.dest] =
                widenToI64(builder.CreateNot(toI1(one(instr)), "lnot"));
            break;
        }

        // ── Calls ─────────────────────────────────────────────────────────────

        case ir::Opcode::Call: {
            // Find or create a declaration for the callee.
            Function* callee = mod.getFunction(instr.label);
            if (!callee) {
                const bool wantRet = (instr.dest != ir::NoReg);
                std::vector<Type*> argTys(instr.operands.size(), i64Ty());
                FunctionType* fty = FunctionType::get(
                    wantRet ? i64Ty() : voidTy(), argTys, /*isVarArg=*/false);
                callee = Function::Create(fty, GlobalValue::ExternalLinkage,
                                          instr.label, mod);
            }
            std::vector<Value*> args;
            args.reserve(instr.operands.size());
            for (const auto& op : instr.operands)
                args.push_back(resolve(op));

            const std::string callName =
                (instr.dest != ir::NoReg) ? "call" : "";
            CallInst* ci = builder.CreateCall(callee, args, callName);
            if (instr.dest != ir::NoReg) regs[instr.dest] = ci;
            break;
        }

        // ── Returns ───────────────────────────────────────────────────────────

        case ir::Opcode::Ret: {
            if (fn->getReturnType()->isVoidTy() || instr.operands.empty()) {
                // Bare `ret` in a non-void function → unreachable dead code.
                if (fn->getReturnType()->isVoidTy())
                    builder.CreateRetVoid();
                else
                    builder.CreateUnreachable();
            } else {
                builder.CreateRet(resolve(instr.operands[0]));
            }
            break;
        }

        // ── Branches ──────────────────────────────────────────────────────────

        case ir::Opcode::Branch: {
            auto it = blocks.find(instr.label);
            if (it != blocks.end())
                builder.CreateBr(it->second);
            break;
        }

        case ir::Opcode::CondBranch: {
            // label = "trueLabel,falseLabel"
            const auto comma = instr.label.find(',');
            if (comma != std::string::npos && !instr.operands.empty()) {
                const std::string trueLbl  = instr.label.substr(0, comma);
                const std::string falseLbl = instr.label.substr(comma + 1);
                Value* cond = toI1(resolve(instr.operands[0]));
                builder.CreateCondBr(cond,
                                      blocks.at(trueLbl),
                                      blocks.at(falseLbl));
            }
            break;
        }

        // ── Cast (Phase 1: identity) ───────────────────────────────────────────

        case ir::Opcode::Cast: {
            if (!instr.operands.empty() && instr.dest != ir::NoReg)
                regs[instr.dest] = resolve(instr.operands[0]);
            break;
        }

        // Phase 3+
        case ir::Opcode::Phi:
            break;

        } // switch
    }

    // ── Lower the entire function ─────────────────────────────────────────────

    void lower() {
        createFunction();
        createBlocks();

        for (const auto& bb : irFn.blocks()) {
            BasicBlock* llvmBB = blocks.at(bb->label());
            builder.SetInsertPoint(llvmBB);

            for (const auto& instr : bb->instructions()) {
                // Do not insert past a terminator (dead code after `ret`).
                if (llvmBB->getTerminator()) break;
                emit(instr);
            }

            // Every block must end with a terminator.
            if (!llvmBB->getTerminator()) {
                if (fn->getReturnType()->isVoidTy())
                    builder.CreateRetVoid();
                else
                    builder.CreateUnreachable();
            }
        }
    }
};

} // anonymous namespace

// ─── LLVMCodeGenerator ───────────────────────────────────────────────────────

LLVMCodeGenerator::LLVMCodeGenerator()
    : ctx_(std::make_unique<LLVMContext>()) {}

LLVMCodeGenerator::~LLVMCodeGenerator() = default;

std::unique_ptr<llvm::Module>
LLVMCodeGenerator::lower(const ir::IRModule& irModule) {
    auto llvmMod = std::make_unique<llvm::Module>(irModule.name(), *ctx_);
    llvmMod->setSourceFileName(irModule.name());

    for (const auto& irFn : irModule.functions()) {
        FunctionLowering fl(*ctx_, *llvmMod, *irFn);
        fl.lower();
    }

    // Verify the generated module.
    std::string errBuf;
    llvm::raw_string_ostream errStream(errBuf);
    if (llvm::verifyModule(*llvmMod, &errStream)) {
        llvm::errs() << "[vcc] LLVM verification failed:\n"
                     << errStream.str() << '\n';
        return nullptr;
    }

    return llvmMod;
}

bool LLVMCodeGenerator::emitIR(const ir::IRModule& irModule,
                                 const std::filesystem::path& outputPath) {
    auto llvmMod = lower(irModule);
    if (!llvmMod) return false;

    std::error_code ec;
    llvm::raw_fd_ostream out(outputPath.string(), ec);
    if (ec) {
        llvm::errs() << "[vcc] cannot open '" << outputPath.string()
                     << "': " << ec.message() << '\n';
        return false;
    }

    llvmMod->print(out, /*AAW=*/nullptr);
    return true;
}

} // namespace vcc::codegen

#else  // VCC_ENABLE_LLVM not defined — provide no-op stubs

#include "vcc/codegen/LLVMCodeGenerator.h"
#include <iostream>

namespace vcc::codegen {

LLVMCodeGenerator::LLVMCodeGenerator()  = default;
LLVMCodeGenerator::~LLVMCodeGenerator() = default;

std::unique_ptr<llvm::Module>
LLVMCodeGenerator::lower(const ir::IRModule&) {
    std::cerr << "[vcc] LLVM backend not enabled (rebuild with -DVCC_ENABLE_LLVM=ON)\n";
    return nullptr;
}

bool LLVMCodeGenerator::emitIR(const ir::IRModule&,
                                 const std::filesystem::path&) {
    std::cerr << "[vcc] LLVM backend not enabled (rebuild with -DVCC_ENABLE_LLVM=ON)\n";
    return false;
}

} // namespace vcc::codegen

#endif // VCC_ENABLE_LLVM
