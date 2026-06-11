#include "vcc/driver/Compiler.h"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <string_view>

// ─── Help / version ───────────────────────────────────────────────────────────

static void printUsage(const char* argv0) {
    std::cout <<
        "Usage: " << argv0 << " [options] <file.v>\n"
        "\n"
        "Compiler pipeline:\n"
        "  Source → Lexer → Parser → AST → Semantic Analysis → IR → LLVM IR\n"
        "\n"
        "Pipeline inspection (stop early and print):\n"
        "  --tokens         Print the token stream and exit\n"
        "  --ast            Print the AST and exit\n"
        "  --ir             Print VCC IR and exit\n"
        "\n"
        "Code generation:\n"
        "  --emit-llvm      Emit LLVM IR to a .ll file  [default when no flag given]\n"
        "  -o <file>        Output file path            (default: output.ll)\n"
        "\n"
        "Other options:\n"
        "  --no-codegen     Run only through semantic analysis (error checking)\n"
        "  --no-color       Disable ANSI colour in diagnostic output\n"
        "  -O<0|1|2|3>      Optimisation level          (default: 0)\n"
        "  -v, --verbose    Show per-stage progress\n"
        "  --version        Print version and exit\n"
        "  -h, --help       Show this help\n"
        "\n"
        "Aliases (legacy):\n"
        "  --dump-tokens    Same as --tokens\n"
        "  --dump-ast       Same as --ast\n"
        "  --dump-ir        Same as --ir\n";
}

static void printVersion() {
    std::cout << "vcc 0.1.0  (V Compiler Collection – LLVM 18 backend)\n";
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    vcc::common::CompilerOptions opts;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help")    { printUsage(argv[0]); return 0; }
        if (arg == "--version")                { printVersion();       return 0; }

        // ── Pipeline inspection flags ─────────────────────────────────────────
        if (arg == "--tokens"      || arg == "--dump-tokens") { opts.dumpTokens = true; continue; }
        if (arg == "--ast"         || arg == "--dump-ast")    { opts.dumpAST    = true; continue; }
        if (arg == "--ir"          || arg == "--dump-ir")     { opts.dumpIR     = true; continue; }

        // ── Code generation flags ─────────────────────────────────────────────
        if (arg == "--emit-llvm")  { opts.emitLLVM   = true; continue; }
        if (arg == "--no-codegen") { opts.noCodegen  = true; continue; }
        if (arg == "--no-color")   { opts.noColor    = true; continue; }

        if (arg == "-v" || arg == "--verbose") { opts.verbose = true; continue; }

        if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "error: -o requires an argument\n";
                return 1;
            }
            opts.outputFile = argv[++i];
            continue;
        }
        if (arg.starts_with("-o") && arg.size() > 2) {
            opts.outputFile = arg.substr(2);
            continue;
        }

        if (arg == "-O0") { opts.optimizationLevel = 0; continue; }
        if (arg == "-O1") { opts.optimizationLevel = 1; continue; }
        if (arg == "-O2") { opts.optimizationLevel = 2; continue; }
        if (arg == "-O3") { opts.optimizationLevel = 3; continue; }

        if (arg.starts_with("-")) {
            std::cerr << "error: unknown option '" << arg << "'\n";
            printUsage(argv[0]);
            return 1;
        }

        opts.inputFiles.push_back(arg);
    }

    if (opts.inputFiles.empty()) {
        std::cerr << "error: no input files\n";
        return 1;
    }

    vcc::driver::Compiler compiler(std::move(opts));
    return compiler.run();
}
