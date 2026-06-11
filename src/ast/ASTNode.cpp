#include "vcc/ast/ASTNode.h"

// ASTNode and its category bases (Decl, Stmt, Expr, TypeNode) are thin
// abstract classes defined entirely in the header.
//
// This translation unit exists so the vcc_ast library target is never empty.
// Future: place ASTDumper, ASTPrinter, or RecursiveASTVisitor implementations
// here once those utilities are added.

namespace vcc::ast {
} // namespace vcc::ast
