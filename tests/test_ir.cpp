#include <gtest/gtest.h>

#include "vcc/common/CompilerContext.h"
#include "vcc/ir/IRGen.h"
#include "vcc/ir/IRInstruction.h"
#include "vcc/ir/IRModule.h"
#include "vcc/ir/IRPrinter.h"
#include "vcc/lexer/Lexer.h"
#include "vcc/parser/Parser.h"
#include "vcc/semantic/SemanticAnalyzer.h"

#include <sstream>

using namespace vcc::ir;
using namespace vcc::common;

// ─── Helpers ─────────────────────────────────────────────────────────────────

/// Parse → semantic-analyse → generate IR from a V source string.
static std::unique_ptr<IRModule> irFrom(const std::string& src) {
    CompilerContext ctx;
    FileID id = ctx.addSource("test.v", src);
    vcc::lexer::Lexer  lex(ctx, id);
    vcc::parser::Parser parser(ctx, lex.tokenize());
    auto mod = parser.parse();
    if (!mod || ctx.diagnostics().hasErrors()) return nullptr;

    vcc::semantic::SemanticAnalyzer sema(ctx);
    sema.analyze(*mod);

    IRGen gen(ctx);
    return gen.generate(*mod);
}

/// Return the IRPrinter text for an IR module.
static std::string irText(const IRModule& m) {
    std::ostringstream oss;
    IRPrinter printer(oss);
    printer.print(m);
    return oss.str();
}

// ─── Opcode names ─────────────────────────────────────────────────────────────

TEST(IRInstructionTest, OpcodeNames) {
    EXPECT_EQ(opcodeName(Opcode::Add),        "add");
    EXPECT_EQ(opcodeName(Opcode::Sub),        "sub");
    EXPECT_EQ(opcodeName(Opcode::Mul),        "mul");
    EXPECT_EQ(opcodeName(Opcode::Div),        "div");
    EXPECT_EQ(opcodeName(Opcode::Mod),        "mod");
    EXPECT_EQ(opcodeName(Opcode::Load),       "load");
    EXPECT_EQ(opcodeName(Opcode::Store),      "store");
    EXPECT_EQ(opcodeName(Opcode::Ret),        "ret");
    EXPECT_EQ(opcodeName(Opcode::Branch),     "br");
    EXPECT_EQ(opcodeName(Opcode::CondBranch), "cbr");
    EXPECT_EQ(opcodeName(Opcode::Call),       "call");
    EXPECT_EQ(opcodeName(Opcode::Alloca),     "alloca");
    EXPECT_EQ(opcodeName(Opcode::Neg),        "neg");
    EXPECT_EQ(opcodeName(Opcode::CmpEq),      "ceq");
    EXPECT_EQ(opcodeName(Opcode::CmpNe),      "cne");
    EXPECT_EQ(opcodeName(Opcode::CmpLt),      "clt");
    EXPECT_EQ(opcodeName(Opcode::CmpGt),      "cgt");
    EXPECT_EQ(opcodeName(Opcode::CmpLe),      "cle");
    EXPECT_EQ(opcodeName(Opcode::CmpGe),      "cge");
    EXPECT_EQ(opcodeName(Opcode::LogAnd),     "land");
    EXPECT_EQ(opcodeName(Opcode::LogOr),      "lor");
    EXPECT_EQ(opcodeName(Opcode::LogNot),     "lnot");
    EXPECT_EQ(opcodeName(Opcode::Copy),       "copy");
    EXPECT_EQ(opcodeName(Opcode::Phi),        "phi");
}

// ─── isTerminator ─────────────────────────────────────────────────────────────

TEST(IRInstructionTest, IsTerminatorRet) {
    IRInstruction instr;
    instr.opcode = Opcode::Ret;
    EXPECT_TRUE(instr.isTerminator());
}

TEST(IRInstructionTest, IsTerminatorBranch) {
    IRInstruction instr;
    instr.opcode = Opcode::Branch;
    EXPECT_TRUE(instr.isTerminator());
}

TEST(IRInstructionTest, IsTerminatorCondBranch) {
    IRInstruction instr;
    instr.opcode = Opcode::CondBranch;
    EXPECT_TRUE(instr.isTerminator());
}

TEST(IRInstructionTest, NonTerminator) {
    for (Opcode op : {Opcode::Add, Opcode::Load, Opcode::Store,
                       Opcode::Call, Opcode::Alloca, Opcode::Copy}) {
        IRInstruction instr;
        instr.opcode = op;
        EXPECT_FALSE(instr.isTerminator()) << opcodeName(op);
    }
}

// ─── Operand factories ────────────────────────────────────────────────────────

TEST(OperandTest, FromReg) {
    auto op = Operand::fromReg(42);
    EXPECT_EQ(op.kind, Operand::Kind::Reg);
    EXPECT_EQ(op.reg,  42u);
}

TEST(OperandTest, FromInt) {
    auto op = Operand::fromInt(-99);
    EXPECT_EQ(op.kind,   Operand::Kind::IntImm);
    EXPECT_EQ(op.intVal, -99);
}

TEST(OperandTest, FromFloat) {
    auto op = Operand::fromFloat(3.14);
    EXPECT_EQ(op.kind,     Operand::Kind::FloatImm);
    EXPECT_DOUBLE_EQ(op.floatVal, 3.14);
}

TEST(OperandTest, FromBool) {
    auto t = Operand::fromBool(true);
    auto f = Operand::fromBool(false);
    EXPECT_EQ(t.kind,   Operand::Kind::BoolImm);
    EXPECT_NE(t.intVal, 0);
    EXPECT_EQ(f.intVal, 0);
}

TEST(OperandTest, FromString) {
    auto op = Operand::fromString("hello");
    EXPECT_EQ(op.kind,   Operand::Kind::StringRef);
    EXPECT_EQ(op.strVal, "hello");
}

// ─── IRBasicBlock ─────────────────────────────────────────────────────────────

TEST(IRBasicBlockTest, StartsEmpty) {
    IRBasicBlock bb("entry");
    EXPECT_EQ(bb.label(), "entry");
    EXPECT_TRUE(bb.instructions().empty());
    EXPECT_FALSE(bb.isTerminated());
}

TEST(IRBasicBlockTest, AppendAndTerminate) {
    IRBasicBlock bb("entry");
    IRInstruction add;
    add.opcode = Opcode::Add;
    add.dest   = 1;
    add.operands = {Operand::fromInt(1), Operand::fromInt(2)};
    bb.append(add);
    EXPECT_FALSE(bb.isTerminated());

    IRInstruction ret;
    ret.opcode = Opcode::Ret;
    bb.append(ret);
    EXPECT_TRUE(bb.isTerminated());
    EXPECT_EQ(bb.instructions().size(), 2u);
}

// ─── IRFunction ───────────────────────────────────────────────────────────────

TEST(IRFunctionTest, AddBlock) {
    IRFunction fn("f");
    EXPECT_EQ(fn.blocks().size(), 0u);
    fn.addBlock("entry");
    EXPECT_EQ(fn.blocks().size(), 1u);
    EXPECT_EQ(fn.entryBlock()->label(), "entry");
}

TEST(IRFunctionTest, RegisterCounter) {
    IRFunction fn("f");
    EXPECT_EQ(fn.nextReg(), 1u);
    EXPECT_EQ(fn.nextReg(), 2u);
    EXPECT_EQ(fn.nextReg(), 3u);
}

// ─── IRModule ─────────────────────────────────────────────────────────────────

TEST(IRModuleTest, AddFunction) {
    IRModule mod("mymod");
    EXPECT_EQ(mod.functions().size(), 0u);
    mod.addFunction("foo");
    mod.addFunction("bar");
    EXPECT_EQ(mod.functions().size(), 2u);
    EXPECT_EQ(mod.functions()[0]->name(), "foo");
    EXPECT_EQ(mod.functions()[1]->name(), "bar");
}

// ─── IRGen – instruction shape tests ─────────────────────────────────────────

TEST(IRGenTest, EmptyFunction) {
    auto m = irFrom("fn f() {}");
    ASSERT_NE(m, nullptr);
    ASSERT_EQ(m->functions().size(), 1u);
    EXPECT_EQ(m->functions()[0]->name(), "f");
    // entry block should have a void ret
    const auto& entry = *m->functions()[0]->entryBlock();
    ASSERT_FALSE(entry.instructions().empty());
    EXPECT_EQ(entry.instructions().back().opcode, Opcode::Ret);
}

TEST(IRGenTest, IntLiteral) {
    auto m = irFrom("fn f() -> i64 { return 42 }");
    ASSERT_NE(m, nullptr);
    const auto& instrs = m->functions()[0]->entryBlock()->instructions();
    // Should have a Copy (immediate) and then a Ret
    bool hasCopy42 = false;
    for (const auto& i : instrs) {
        if (i.opcode == Opcode::Copy && !i.operands.empty() &&
            i.operands[0].kind == Operand::Kind::IntImm &&
            i.operands[0].intVal == 42)
            hasCopy42 = true;
    }
    EXPECT_TRUE(hasCopy42) << "Expected 'copy 42' instruction";
}

TEST(IRGenTest, AddExpression) {
    auto m = irFrom("fn f(a: i64, b: i64) -> i64 { return a + b }");
    ASSERT_NE(m, nullptr);
    // Should emit alloca×2, load×2, add, ret
    const auto& fn = *m->functions()[0];
    bool hasAdd = false;
    for (const auto& bb : fn.blocks())
        for (const auto& i : bb->instructions())
            if (i.opcode == Opcode::Add) hasAdd = true;
    EXPECT_TRUE(hasAdd);
}

TEST(IRGenTest, SubExpression) {
    auto m = irFrom("fn f(a: i64, b: i64) -> i64 { return a - b }");
    ASSERT_NE(m, nullptr);
    bool hasSub = false;
    for (const auto& bb : m->functions()[0]->blocks())
        for (const auto& i : bb->instructions())
            if (i.opcode == Opcode::Sub) hasSub = true;
    EXPECT_TRUE(hasSub);
}

TEST(IRGenTest, MulExpression) {
    auto m = irFrom("fn f(a: i64, b: i64) -> i64 { return a * b }");
    ASSERT_NE(m, nullptr);
    bool hasMul = false;
    for (const auto& bb : m->functions()[0]->blocks())
        for (const auto& i : bb->instructions())
            if (i.opcode == Opcode::Mul) hasMul = true;
    EXPECT_TRUE(hasMul);
}

TEST(IRGenTest, DivExpression) {
    auto m = irFrom("fn f(a: i64, b: i64) -> i64 { return a / b }");
    ASSERT_NE(m, nullptr);
    bool hasDiv = false;
    for (const auto& bb : m->functions()[0]->blocks())
        for (const auto& i : bb->instructions())
            if (i.opcode == Opcode::Div) hasDiv = true;
    EXPECT_TRUE(hasDiv);
}

TEST(IRGenTest, VarDeclAllocaStore) {
    auto m = irFrom("fn f() { let x = 10 }");
    ASSERT_NE(m, nullptr);
    const auto& fn = *m->functions()[0];
    bool hasAlloca = false, hasStore = false;
    for (const auto& bb : fn.blocks())
        for (const auto& i : bb->instructions()) {
            if (i.opcode == Opcode::Alloca) hasAlloca = true;
            if (i.opcode == Opcode::Store)  hasStore  = true;
        }
    EXPECT_TRUE(hasAlloca) << "VarDecl should emit alloca";
    EXPECT_TRUE(hasStore)  << "VarDecl with initializer should emit store";
}

TEST(IRGenTest, LoadStoreSequence) {
    // t1 = load x; t2 = load y; t3 = add t1 t2; store t3 z
    auto m = irFrom(
        "fn f() {\n"
        "  let x = 1\n"
        "  let y = 2\n"
        "  var z = x + y\n"
        "}\n"
    );
    ASSERT_NE(m, nullptr);
    const auto& fn = *m->functions()[0];
    int loads = 0, stores = 0, adds = 0;
    for (const auto& bb : fn.blocks())
        for (const auto& i : bb->instructions()) {
            if (i.opcode == Opcode::Load)  ++loads;
            if (i.opcode == Opcode::Store) ++stores;
            if (i.opcode == Opcode::Add)   ++adds;
        }
    EXPECT_GE(loads,  2) << "Expected at least 2 loads for x and y";
    EXPECT_GE(stores, 3) << "Expected stores for x, y, and z";
    EXPECT_EQ(adds,   1) << "Expected exactly 1 add instruction";
}

TEST(IRGenTest, CallInstruction) {
    auto m = irFrom("fn helper() -> i64 { return 1 }\nfn f() { helper() }");
    ASSERT_NE(m, nullptr);
    const auto& caller = *m->functions()[1];
    bool hasCall = false;
    for (const auto& bb : caller.blocks())
        for (const auto& i : bb->instructions())
            if (i.opcode == Opcode::Call && i.label == "helper")
                hasCall = true;
    EXPECT_TRUE(hasCall) << "Call instruction should use function name as label";
}

TEST(IRGenTest, ReturnInstruction) {
    auto m = irFrom("fn f() -> i64 { return 7 }");
    ASSERT_NE(m, nullptr);
    bool hasRet = false;
    for (const auto& bb : m->functions()[0]->blocks())
        for (const auto& i : bb->instructions())
            if (i.opcode == Opcode::Ret) hasRet = true;
    EXPECT_TRUE(hasRet);
}

TEST(IRGenTest, IfStmtGeneratesCondBranch) {
    auto m = irFrom("fn f(x: i64) { if x { let a = 1 } }");
    ASSERT_NE(m, nullptr);
    const auto& fn = *m->functions()[0];
    bool hasCbr = false;
    for (const auto& bb : fn.blocks())
        for (const auto& i : bb->instructions())
            if (i.opcode == Opcode::CondBranch) hasCbr = true;
    EXPECT_TRUE(hasCbr);
}

TEST(IRGenTest, WhileStmtGeneratesLoop) {
    auto m = irFrom("fn f(n: i64) { while n { let a = 1 } }");
    ASSERT_NE(m, nullptr);
    const auto& fn = *m->functions()[0];
    int branches = 0;
    for (const auto& bb : fn.blocks())
        for (const auto& i : bb->instructions())
            if (i.opcode == Opcode::Branch || i.opcode == Opcode::CondBranch)
                ++branches;
    EXPECT_GE(branches, 3) << "while loop needs br+cbr+back-edge";
}

TEST(IRGenTest, MultipleBlocks) {
    auto m = irFrom("fn f(x: i64) { if x { return } }");
    ASSERT_NE(m, nullptr);
    // Should have entry, then, merge blocks at minimum
    EXPECT_GE(m->functions()[0]->blocks().size(), 2u);
}

// ─── IRPrinter ────────────────────────────────────────────────────────────────

TEST(IRPrinterTest, ModuleBanner) {
    auto m = irFrom("fn f() {}");
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);
    EXPECT_NE(out.find("VCC IR"), std::string::npos);
}

TEST(IRPrinterTest, FunctionHeader) {
    auto m = irFrom("fn myFunc() {}");
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);
    EXPECT_NE(out.find("fn myFunc"), std::string::npos);
}

TEST(IRPrinterTest, EntryLabel) {
    auto m = irFrom("fn f() {}");
    ASSERT_NE(m, nullptr);
    EXPECT_NE(irText(*m).find("entry:"), std::string::npos);
}

TEST(IRPrinterTest, AllocaUsesVarName) {
    auto m = irFrom("fn f() { let count = 0 }");
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);
    // The alloca for 'count' should appear with the name "count"
    EXPECT_NE(out.find("count = alloca"), std::string::npos);
}

TEST(IRPrinterTest, LoadInstruction) {
    auto m = irFrom("fn f() { let x = 1 let y = x }");
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);
    // Should contain a load from x
    EXPECT_NE(out.find("load"), std::string::npos);
    EXPECT_NE(out.find("x"),    std::string::npos);
}

TEST(IRPrinterTest, StoreInstruction) {
    auto m = irFrom("fn f() { let x = 42 }");
    ASSERT_NE(m, nullptr);
    EXPECT_NE(irText(*m).find("store"), std::string::npos);
}

TEST(IRPrinterTest, AddInstruction) {
    auto m = irFrom("fn f(a: i64, b: i64) -> i64 { return a + b }");
    ASSERT_NE(m, nullptr);
    EXPECT_NE(irText(*m).find("add"), std::string::npos);
}

TEST(IRPrinterTest, RetInstruction) {
    auto m = irFrom("fn f() -> i64 { return 1 }");
    ASSERT_NE(m, nullptr);
    EXPECT_NE(irText(*m).find("ret"), std::string::npos);
}

TEST(IRPrinterTest, CallInstruction) {
    auto m = irFrom("fn g() {}\nfn f() { g() }");
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);
    EXPECT_NE(out.find("call g"), std::string::npos);
}

TEST(IRPrinterTest, BranchInstruction) {
    auto m = irFrom("fn f(x: i64) { if x { } }");
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);
    EXPECT_NE(out.find("cbr"), std::string::npos);
}

TEST(IRPrinterTest, FullLoadStoreAddExample) {
    // Matches the requirement example:
    //   t1 = load x
    //   t2 = load y
    //   t3 = add t1 t2
    //   store t3 z
    auto m = irFrom(
        "fn f() {\n"
        "  let x = 1\n"
        "  let y = 2\n"
        "  var z = x + y\n"
        "}\n"
    );
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);
    EXPECT_NE(out.find("load"), std::string::npos);
    EXPECT_NE(out.find("add"),  std::string::npos);
    EXPECT_NE(out.find("store"), std::string::npos);
    EXPECT_NE(out.find("x"),    std::string::npos);
    EXPECT_NE(out.find("y"),    std::string::npos);
    EXPECT_NE(out.find("z"),    std::string::npos);
}

TEST(IRPrinterTest, TempNamesAreSequential) {
    // Temporaries that are not alloca results should be t1, t2, …
    auto m = irFrom("fn f(a: i64, b: i64) -> i64 { return a + b }");
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);
    EXPECT_NE(out.find("t1"), std::string::npos);
    EXPECT_NE(out.find("t2"), std::string::npos);
}

TEST(IRPrinterTest, PrintToOstream) {
    // Verify that IRModule::print(ostream) delegates to IRPrinter correctly.
    auto m = irFrom("fn f() -> i64 { return 0 }");
    ASSERT_NE(m, nullptr);
    std::ostringstream oss;
    m->print(oss);
    EXPECT_NE(oss.str().find("fn f"), std::string::npos);
}

// ─── Phase 5: AST→IR lowering correctness ────────────────────────────────────

/// Requirement: `let z = x + y` must produce the exact pattern
///   t1 = load x;  t2 = load y;  t3 = add t1 t2;  store t3 z
/// (The requirement only mandates the pattern, not the specific tN numbers.)
TEST(IRGenTest, RequirementExample_LoadAddStore) {
    auto m = irFrom(
        "fn f(x: i64, y: i64) {\n"
        "  let z = x + y\n"
        "}\n"
    );
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);

    EXPECT_NE(out.find("x = alloca"),  std::string::npos) << out;
    EXPECT_NE(out.find("y = alloca"),  std::string::npos) << out;
    EXPECT_NE(out.find("z = alloca"),  std::string::npos) << out;
    EXPECT_NE(out.find("load x"),      std::string::npos) << out;
    EXPECT_NE(out.find("load y"),      std::string::npos) << out;
    EXPECT_NE(out.find("add"),         std::string::npos) << out;
    EXPECT_NE(out.find("store"),       std::string::npos) << out;
}

/// Param instruction: each formal parameter must emit `param N` and
/// the alloca slot must be initialised via a store.
TEST(IRGenTest, ParameterCallingConvention) {
    auto m = irFrom("fn f(x: i64, y: i64) -> i64 { return x + y }");
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);

    // alloca slots for both parameters
    EXPECT_NE(out.find("x = alloca"),  std::string::npos) << out;
    EXPECT_NE(out.find("y = alloca"),  std::string::npos) << out;
    // param instructions for both
    EXPECT_NE(out.find("param 0"),     std::string::npos) << out;
    EXPECT_NE(out.find("param 1"),     std::string::npos) << out;
    // slots must be initialised (two stores before the body)
    size_t first_store  = out.find("store");
    size_t second_store = out.find("store", first_store + 1);
    EXPECT_NE(first_store,  std::string::npos) << "missing first param store\n"  << out;
    EXPECT_NE(second_store, std::string::npos) << "missing second param store\n" << out;
}

/// IRPrinter: `param N` instruction must appear in printed output.
TEST(IRPrinterTest, ParamOpcodeInOutput) {
    auto m = irFrom("fn g(a: i64) -> i64 { return a }");
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);
    EXPECT_NE(out.find("param 0"), std::string::npos) << out;
}

/// char literals should be lowered to a copy of the code-point value.
TEST(IRGenTest, CharLiteralLowering) {
    auto m = irFrom("fn f() -> i64 { let c = 'A'\n return 0 }");
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);
    // The char 'A' is U+0041 = 65.  The IR should contain a copy with value 65.
    EXPECT_NE(out.find("copy"), std::string::npos) << out;
    EXPECT_NE(out.find("65"),   std::string::npos) << out;
}

/// nil literal should lower to a copy 0.
TEST(IRGenTest, NilLiteralLowering) {
    auto m = irFrom("fn f() { let n = nil }");
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);
    EXPECT_NE(out.find("copy"), std::string::npos) << out;
}

/// Compound assignment `x += 1` should emit load → add → store.
TEST(IRGenTest, CompoundAssignmentAddAssign) {
    auto m = irFrom(
        "fn f() {\n"
        "  var x = 0\n"
        "  x += 1\n"
        "}\n"
    );
    ASSERT_NE(m, nullptr);
    const std::string out = irText(*m);

    EXPECT_NE(out.find("load x"), std::string::npos) << out;
    EXPECT_NE(out.find("add"),    std::string::npos) << out;
    EXPECT_NE(out.find("store"),  std::string::npos) << out;
}

