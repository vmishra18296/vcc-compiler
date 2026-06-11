#pragma once

/// @file AST.h
/// Umbrella header – include this single file to get the entire VCC AST.
///
/// Complete hierarchy
/// ══════════════════
///
///   Node  (= ASTNode)
///   ├── Decl
///   │   ├── Program          (= ModuleDecl)  – top-level compilation unit
///   │   ├── ImportDecl                       – import std.io
///   │   ├── FunctionDecl                     – fn foo(…) -> T { … }
///   │   ├── ParameterDecl                    – single formal parameter
///   │   ├── VariableDecl     (= VarDecl)     – let / var / const binding
///   │   ├── StructDecl                       – struct Point { … }
///   │   ├── FieldDecl                        – single struct field
///   │   ├── EnumDecl                         – enum Color { … }
///   │   ├── EnumVariantDecl                  – single enum variant
///   │   └── TypeAliasDecl                    – type Alias = …
///   │
///   ├── Statement  (= Stmt)
///   │   ├── BlockStmt                        – { s1; s2; … }
///   │   ├── ExprStmt                         – expr (value discarded)
///   │   ├── DeclStmt                         – wraps a Decl inside a block
///   │   ├── ReturnStmt                       – return [expr]
///   │   ├── IfStmt                           – if cond { … } [else { … }]
///   │   ├── WhileStmt                        – while cond { … }
///   │   ├── ForStmt                          – for x in iter { … }
///   │   ├── BreakStmt                        – break
///   │   └── ContinueStmt                     – continue
///   │
///   ├── Expression  (= Expr)
///   │   ├── BinaryExpr                       – lhs op rhs
///   │   ├── UnaryExpr                        – op operand
///   │   ├── AssignExpr                       – lhs = rhs / += / -= / …
///   │   ├── CallExpr                         – callee(args…)
///   │   ├── IndexExpr                        – base[index]
///   │   ├── MemberExpr                       – object.field
///   │   ├── CastExpr                         – expr as Type
///   │   ├── IdentifierExpr   (= IdentExpr)   – bare identifier reference
///   │   └── LiteralExpr      (abstract base)
///   │       ├── IntLiteralExpr               – 42, 0xFF, 0b1010, 0o77
///   │       ├── FloatLiteralExpr             – 3.14, 1.0e-9
///   │       ├── StringLiteralExpr            – "hello"
///   │       ├── CharLiteralExpr              – 'a'
///   │       ├── BoolLiteralExpr              – true / false
///   │       └── NilLiteralExpr               – nil
///   │
///   └── TypeNode
///       ├── NamedTypeNode                    – int, float, MyStruct
///       ├── PointerTypeNode                  – *T
///       ├── SliceTypeNode                    – []T
///       ├── ArrayTypeNode                    – [N]T
///       └── FunctionTypeNode                 – fn(A, B) -> C
///
/// Visitor pattern
/// ───────────────
///   ASTVisitor        – pure-virtual; guaranteed coverage of all node kinds
///   ASTVisitorBase    – no-op defaults; derive and override only what you need
///
/// Ownership
/// ─────────
///   All nodes are heap-allocated via std::make_unique.
///   Every parent owns its children through std::unique_ptr.
///   No raw new/delete; no shared ownership (except where explicitly noted).
///
/// Source locations
/// ────────────────
///   Every ASTNode carries a common::SourceRange { start, end } pair.
///   SourceLocation holds { FileID, line, column }.

#include "ASTNode.h"       // Node, Statement, Expression (+ ASTNode, Decl, Stmt, Expr, TypeNode)
#include "ASTVisitor.h"    // ASTVisitor, ASTVisitorBase
#include "Declarations.h"  // Program, VariableDecl (+ all Decl subclasses)
#include "Expressions.h"   // LiteralExpr, IdentifierExpr (+ all Expr subclasses)
#include "Statements.h"    // All Stmt subclasses
#include "Types.h"         // All TypeNode subclasses
