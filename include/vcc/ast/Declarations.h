#pragma once

#include "ASTNode.h"
#include "ASTVisitor.h"
#include "Types.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

/// @file Declarations.h
/// Declaration AST nodes for the V language.
///
/// Hierarchy
/// ─────────
///   Decl
///   ├── ModuleDecl      – top-level compilation unit
///   ├── ImportDecl      – import statement
///   ├── FunctionDecl    – fn foo(…) -> T { … }
///   ├── ParameterDecl   – a single function parameter
///   ├── StructDecl      – struct Foo { … }
///   ├── FieldDecl       – a single struct field
///   ├── EnumDecl        – enum Color { … }
///   ├── EnumVariantDecl – a single enum variant
///   ├── VarDecl         – let/var/const binding
///   └── TypeAliasDecl   – type Alias = …
///
/// Future extensions
/// ─────────────────
///  • InterfaceDecl and ImplDecl for trait-like polymorphism.
///  • GenericParamDecl for generics (Phase 4).
///  • ModuleDecl linking across multiple files.

namespace vcc::ast {

// ─── ModuleDecl ───────────────────────────────────────────────────────────────

/// Root node of a parsed V source file.
/// Owns all top-level declarations.
class ModuleDecl : public Decl {
public:
    explicit ModuleDecl(std::string name, common::SourceRange range = {})
        : Decl(range), name_(std::move(name)) {}

    [[nodiscard]] const std::string&                    name()  const noexcept { return name_; }
    [[nodiscard]] const std::vector<std::unique_ptr<Decl>>& decls() const noexcept { return decls_; }

    void addDecl(std::unique_ptr<Decl> d) { decls_.push_back(std::move(d)); }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "ModuleDecl"; }

private:
    std::string                         name_;
    std::vector<std::unique_ptr<Decl>>  decls_;
};

// ─── ImportDecl ───────────────────────────────────────────────────────────────

/// import std.io
/// import math as m
class ImportDecl : public Decl {
public:
    ImportDecl(std::string path,
               std::optional<std::string> alias,
               common::SourceRange range = {})
        : Decl(range), path_(std::move(path)), alias_(std::move(alias)) {}

    [[nodiscard]] const std::string&                path()  const noexcept { return path_; }
    [[nodiscard]] const std::optional<std::string>& alias() const noexcept { return alias_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "ImportDecl"; }

private:
    std::string                path_;
    std::optional<std::string> alias_;
};

// ─── ParameterDecl ────────────────────────────────────────────────────────────

/// A single formal parameter: `name: Type`.
class ParameterDecl : public Decl {
public:
    ParameterDecl(std::string              name,
                  std::unique_ptr<TypeNode> type,
                  bool                     isMut,
                  common::SourceRange      range = {})
        : Decl(range),
          name_(std::move(name)),
          type_(std::move(type)),
          isMut_(isMut) {}

    [[nodiscard]] const std::string& name()  const noexcept { return name_; }
    [[nodiscard]] const TypeNode*    type()  const noexcept { return type_.get(); }
    [[nodiscard]] bool               isMut() const noexcept { return isMut_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "ParameterDecl"; }

private:
    std::string               name_;
    std::unique_ptr<TypeNode> type_;
    bool                      isMut_;
};

// ─── FunctionDecl ─────────────────────────────────────────────────────────────

/// fn [pub] name(params) [-> ReturnType] { body }
class FunctionDecl : public Decl {
public:
    FunctionDecl(std::string                              name,
                 std::vector<std::unique_ptr<ParameterDecl>> params,
                 std::unique_ptr<TypeNode>                returnType,
                 std::unique_ptr<Stmt>                    body,
                 bool                                     isPub,
                 common::SourceRange                      range = {})
        : Decl(range),
          name_(std::move(name)),
          params_(std::move(params)),
          returnType_(std::move(returnType)),
          body_(std::move(body)),
          isPub_(isPub) {}

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::vector<std::unique_ptr<ParameterDecl>>& params() const noexcept { return params_; }
    [[nodiscard]] const TypeNode* returnType() const noexcept { return returnType_.get(); }
    [[nodiscard]] const Stmt*     body()       const noexcept { return body_.get(); }
    [[nodiscard]] bool            isPub()      const noexcept { return isPub_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "FunctionDecl"; }

private:
    std::string                                  name_;
    std::vector<std::unique_ptr<ParameterDecl>>  params_;
    std::unique_ptr<TypeNode>                    returnType_;  ///< nullptr = unit
    std::unique_ptr<Stmt>                        body_;
    bool                                         isPub_;
};

// ─── FieldDecl ────────────────────────────────────────────────────────────────

/// A single field inside a struct: `pub name: Type`.
class FieldDecl : public Decl {
public:
    FieldDecl(std::string              name,
              std::unique_ptr<TypeNode> type,
              bool                     isPub,
              common::SourceRange      range = {})
        : Decl(range), name_(std::move(name)), type_(std::move(type)), isPub_(isPub) {}

    [[nodiscard]] const std::string& name()  const noexcept { return name_; }
    [[nodiscard]] const TypeNode*    type()  const noexcept { return type_.get(); }
    [[nodiscard]] bool               isPub() const noexcept { return isPub_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "FieldDecl"; }

private:
    std::string               name_;
    std::unique_ptr<TypeNode> type_;
    bool                      isPub_;
};

// ─── StructDecl ───────────────────────────────────────────────────────────────

/// struct [pub] Name { fields }
class StructDecl : public Decl {
public:
    StructDecl(std::string                           name,
               std::vector<std::unique_ptr<FieldDecl>> fields,
               bool                                  isPub,
               common::SourceRange                   range = {})
        : Decl(range),
          name_(std::move(name)),
          fields_(std::move(fields)),
          isPub_(isPub) {}

    [[nodiscard]] const std::string& name()   const noexcept { return name_; }
    [[nodiscard]] const std::vector<std::unique_ptr<FieldDecl>>& fields() const noexcept { return fields_; }
    [[nodiscard]] bool isPub() const noexcept { return isPub_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "StructDecl"; }

private:
    std::string                             name_;
    std::vector<std::unique_ptr<FieldDecl>> fields_;
    bool                                    isPub_;
};

// ─── EnumVariantDecl ──────────────────────────────────────────────────────────

/// A single variant inside an enum.  Optionally carries associated types.
class EnumVariantDecl : public Decl {
public:
    EnumVariantDecl(std::string                             name,
                    std::vector<std::unique_ptr<TypeNode>>  fields,
                    common::SourceRange                     range = {})
        : Decl(range), name_(std::move(name)), fields_(std::move(fields)) {}

    [[nodiscard]] const std::string& name()   const noexcept { return name_; }
    [[nodiscard]] const std::vector<std::unique_ptr<TypeNode>>& fields() const noexcept { return fields_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "EnumVariantDecl"; }

private:
    std::string                            name_;
    std::vector<std::unique_ptr<TypeNode>> fields_;
};

// ─── EnumDecl ─────────────────────────────────────────────────────────────────

/// enum [pub] Name { Variant1, Variant2(T), … }
class EnumDecl : public Decl {
public:
    EnumDecl(std::string                                  name,
             std::vector<std::unique_ptr<EnumVariantDecl>> variants,
             bool                                         isPub,
             common::SourceRange                          range = {})
        : Decl(range),
          name_(std::move(name)),
          variants_(std::move(variants)),
          isPub_(isPub) {}

    [[nodiscard]] const std::string& name()     const noexcept { return name_; }
    [[nodiscard]] const std::vector<std::unique_ptr<EnumVariantDecl>>& variants() const noexcept { return variants_; }
    [[nodiscard]] bool isPub() const noexcept { return isPub_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "EnumDecl"; }

private:
    std::string                                   name_;
    std::vector<std::unique_ptr<EnumVariantDecl>> variants_;
    bool                                          isPub_;
};

// ─── VarDecl ─────────────────────────────────────────────────────────────────

/// let / var / const binding with optional type annotation and initialiser.
enum class VarKind { Let, Var, Const };

class VarDecl : public Decl {
public:
    VarDecl(VarKind                  kind,
            std::string              name,
            std::unique_ptr<TypeNode> type,      ///< nullptr if inferred
            std::unique_ptr<Expr>    initializer, ///< nullptr if absent (error for const)
            common::SourceRange      range = {})
        : Decl(range),
          varKind_(kind),
          name_(std::move(name)),
          type_(std::move(type)),
          initializer_(std::move(initializer)) {}

    [[nodiscard]] VarKind           varKind()     const noexcept { return varKind_; }
    [[nodiscard]] const std::string& name()       const noexcept { return name_; }
    [[nodiscard]] const TypeNode*   type()        const noexcept { return type_.get(); }
    [[nodiscard]] const Expr*       initializer() const noexcept { return initializer_.get(); }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "VarDecl"; }

private:
    VarKind                   varKind_;
    std::string               name_;
    std::unique_ptr<TypeNode> type_;
    std::unique_ptr<Expr>     initializer_;
};

// ─── TypeAliasDecl ────────────────────────────────────────────────────────────

/// type MyInt = i32
class TypeAliasDecl : public Decl {
public:
    TypeAliasDecl(std::string              name,
                  std::unique_ptr<TypeNode> aliased,
                  bool                     isPub,
                  common::SourceRange      range = {})
        : Decl(range), name_(std::move(name)), aliased_(std::move(aliased)), isPub_(isPub) {}

    [[nodiscard]] const std::string& name()    const noexcept { return name_; }
    [[nodiscard]] const TypeNode*    aliased() const noexcept { return aliased_.get(); }
    [[nodiscard]] bool               isPub()   const noexcept { return isPub_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "TypeAliasDecl"; }

private:
    std::string               name_;
    std::unique_ptr<TypeNode> aliased_;
    bool                      isPub_;
};

// ─── Canonical name aliases ───────────────────────────────────────────────────

/// ModuleDecl is the top-level "Program" node produced by the parser.
using Program = ModuleDecl;

/// VarDecl is also known as VariableDecl in the requirement spec.
using VariableDecl = VarDecl;

} // namespace vcc::ast
