#include "vcc/ast/ASTDumper.h"
#include "vcc/common/CompilerContext.h"
#include "vcc/lexer/Lexer.h"
#include "vcc/parser/Parser.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace vcc;
using namespace vcc::common;

// ─── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: vcc-dump-ast <file.v>\n";
        return 1;
    }

    const std::filesystem::path path = argv[1];
    std::ifstream ifs(path);
    if (!ifs) {
        std::cerr << "error: cannot open '" << path.string() << "'\n";
        return 1;
    }

    std::ostringstream buf;
    buf << ifs.rdbuf();

    CompilerContext ctx;
    FileID id = ctx.addSource(path, buf.str());

    lexer::Lexer lex(ctx, id);
    auto tokens = lex.tokenize();

    if (ctx.diagnostics().hasErrors()) {
        ctx.diagnostics().printAll(std::cerr, path.string());
        return 1;
    }

    parser::Parser parser(ctx, std::move(tokens));
    auto module = parser.parse();

    ctx.diagnostics().printAll(std::cerr, path.string());
    if (!module || ctx.diagnostics().hasErrors()) return 1;

    ast::ASTDumper dumper;
    dumper.dump(*module);


    return 0;
}
