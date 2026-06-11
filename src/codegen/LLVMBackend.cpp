#include "vcc/codegen/LLVMBackend.h"

namespace vcc::codegen {

LLVMBackend::LLVMBackend(common::CompilerContext& ctx) : ctx_(ctx) {}

bool LLVMBackend::generate([[maybe_unused]] const ir::IRModule& module,
                            [[maybe_unused]] const std::filesystem::path& outPath) {
    // ─── Phase 3 stub ────────────────────────────────────────────────────────
    // Full implementation plan:
    //
    //  1. Create an llvm::LLVMContext and llvm::Module.
    //  2. Walk the VCC IRModule:
    //     a. For each IRFunction, create an llvm::Function.
    //     b. For each IRBasicBlock, create an llvm::BasicBlock.
    //     c. For each IRInstruction, emit the corresponding llvm::Instruction
    //        using an llvm::IRBuilder.
    //  3. Verify the module (llvm::verifyModule).
    //  4. Run the optimisation pipeline (llvm::PassBuilder).
    //  5. Emit object code via llvm::TargetMachine::addPassesToEmitFile.
    //
    ctx_.diagnostics().note(
        common::SourceLocation::invalid(),
        "LLVM backend is not yet implemented (Phase 3); "
        "use --dump-ir to inspect VCC IR output");
    return false;
}

} // namespace vcc::codegen
