#include "vcc/driver/Compiler.h"

#include "vcc/ast/ASTDumper.h"
#include "vcc/codegen/LLVMCodeGenerator.h"
#include "vcc/ir/IRGen.h"
#include "vcc/ir/IRPrinter.h"
#include "vcc/lexer/Lexer.h"
#include "vcc/parser/Parser.h"
#include "vcc/semantic/SemanticAnalyzer.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unistd.h>

namespace vcc::driver {

Compiler::Compiler(common::CompilerOptions opts)
    : ctx_(std::move(opts)) {}

// --- run ---------------------------------------------------------------------

int Compiler::run() {
    bool ok = true;
    for (const auto& path : ctx_.options().inputFiles)
        ok &= compileFile(path);

    // Pretty-print all accumulated diagnostics to stderr.
    // Use ANSI colours when stderr is a TTY and --no-color was not passed.
    const bool useColor = !ctx_.options().noColor
                          && isatty(STDERR_FILENO) != 0;
    ctx_.diagnostics().printAllPretty(
        std::cerr,
        [&](common::FileID id) {
            return std::make_pair(ctx_.sourceText(id), ctx_.sourcePath(id));
        },
        useColor);

    return ok ? 0 : 1;
}

// --- Internal helpers --------------------------------------------------------

namespace {

/// Log a pipeline-stage banner when verbose mode is active.
void logStage(const common::CompilerOptions& opts,
              int n, int total, std::string_view description) {
    if (opts.verbose)
        std::cerr << "vcc: [" << n << '/' << total << "] " << description << '\n';
}

/// Print a formatted token table to stdout.
void printTokenTable(const std::vector<common::Token>& tokens) {
    constexpr int kKindW  = 20;
    constexpr int kValueW = 28;

    std::cout << std::left
              << std::setw(kKindW)  << "TOKEN"
              << std::setw(kValueW) << "VALUE"
              << "LOCATION\n"
              << std::string(kKindW + kValueW + 10, '-') << '\n';

    for (const auto& tok : tokens) {
        if (tok.kind == common::TokenKind::Eof) break;
        const std::string loc =
            std::to_string(tok.location.line) + ':' +
            std::to_string(tok.location.column);
        std::cout << std::left
                  << std::setw(kKindW)  << common::tokenTypeName(tok.kind)
                  << std::setw(kValueW) << tok.lexeme
                  << loc << '\n';
    }
}

/// Resolve the LLVM IR output path from CompilerOptions.
/// Honours an explicit -o <path>.ll; otherwise defaults to "output.ll".
std::filesystem::path resolveOutputPath(const common::CompilerOptions& opts) {
    const auto& p = opts.outputFile;
    if (p != std::filesystem::path{"a.out"} && p.extension() == ".ll")
        return p;
    return "output.ll";
}

} // namespace

// --- compileFile -------------------------------------------------------------
//
// Pipeline
// --------
//   Stage 1 -- Read source text
//   Stage 2 -- Lex   -> token stream       [--tokens: print table and stop]
//   Stage 3 -- Parse -> AST                [--ast:    print tree  and stop]
//   Stage 4 -- Semantic analysis
//   Stage 5 -- IR generation               [--ir:     print IR    and stop]
//   Stage 6 -- LLVM lowering -> output.ll  [default or --emit-llvm]

bool Compiler::compileFile(const std::filesystem::path& path) {
    const auto& opts = ctx_.options();
    constexpr int kStages = 6;

    // -- Stage 1: Read source -------------------------------------------------
    logStage(opts, 1, kStages, "reading '" + path.string() + "'");

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        ctx_.diagnostics().fatal(common::SourceLocation::invalid(),
            "cannot open input file '" + path.string() + "'");
        return false;
    }
    std::ostringstream buf;
    buf << ifs.rdbuf();
    const common::FileID fileId = ctx_.addSource(path, buf.str());

    // -- Stage 2: Lex ---------------------------------------------------------
    logStage(opts, 2, kStages, "lexing");

    lexer::Lexer lex(ctx_, fileId);
    auto tokens = lex.tokenize();

    if (opts.dumpTokens) {
        printTokenTable(tokens);
        return !ctx_.diagnostics().hasErrors();
    }

    if (ctx_.diagnostics().hasErrors()) return false;

    // -- Stage 3: Parse -------------------------------------------------------
    logStage(opts, 3, kStages, "parsing");

    parser::Parser parser(ctx_, std::move(tokens));
    auto module = parser.parse();
    if (!module || ctx_.diagnostics().hasErrors()) return false;

    if (opts.dumpAST) {
        ast::ASTDumper dumper(std::cout);
        dumper.dump(*module);
        return true;
    }

    // -- Stage 4: Semantic analysis -------------------------------------------
    logStage(opts, 4, kStages, "semantic analysis");

    semantic::SemanticAnalyzer sema(ctx_);
    if (!sema.analyze(*module)) return false;

    if (opts.noCodegen) {
        if (opts.verbose)
            std::cerr << "vcc: ok  (--no-codegen: stopped after analysis)\n";
        return true;
    }

    // -- Stage 5: IR generation -----------------------------------------------
    logStage(opts, 5, kStages, "IR generation");

    ir::IRGen irgen(ctx_);
    auto irModule = irgen.generate(*module);

    if (opts.dumpIR) {
        ir::IRPrinter printer(std::cout);
        printer.print(*irModule);
        return true;
    }

    // -- Stage 6: LLVM lowering -----------------------------------------------
    // Runs when --emit-llvm is passed explicitly OR when no inspection flag is
    // active (i.e. the default "vcc file.v" invocation produces output.ll).

    const auto outPath = resolveOutputPath(opts);
    logStage(opts, 6, kStages, "LLVM lowering -> " + outPath.string());

    codegen::LLVMCodeGenerator llvmGen;
    const bool ok = llvmGen.emitIR(*irModule, outPath);

    if (ok)
        std::cout << "vcc: wrote '" << outPath.string() << "'\n";

    return ok;
}

} // namespace vcc::driver
