#pragma once

#include "ASTNode.h"
#include "ASTVisitor.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

/// @file Types.h
/// Type-annotation AST nodes.
///
/// These nodes represent what the programmer wrote in a type position,
/// e.g. `i32`, `*mut Foo`, `[u8]`, `(i32, bool) -> str`.
///
/// Future extensions
/// ─────────────────
///  • Generic instantiation nodes: `Vec<i32>`.
///  • Tuple types:  `(i32, bool)`.
///  • Optional shorthand:  `?T`  →  `Option<T>`.

namespace vcc::ast {

// ─── NamedTypeNode ────────────────────────────────────────────────────────────

/// A simple or qualified type name, e.g. `i32`, `std::String`.
class NamedTypeNode : public TypeNode {
public:
    explicit NamedTypeNode(std::string name, common::SourceRange range = {})
        : TypeNode(range), name_(std::move(name)) {}

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "NamedTypeNode"; }

private:
    std::string name_;
};

// ─── PointerTypeNode ──────────────────────────────────────────────────────────

/// A pointer-to-type, e.g. `*T` or `*mut T`.
class PointerTypeNode : public TypeNode {
public:
    PointerTypeNode(std::unique_ptr<TypeNode> inner,
                    bool                     isMut,
                    common::SourceRange      range = {})
        : TypeNode(range), inner_(std::move(inner)), isMut_(isMut) {}

    [[nodiscard]] const TypeNode& inner() const noexcept { return *inner_; }
    [[nodiscard]] bool            isMut() const noexcept { return isMut_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "PointerTypeNode"; }

private:
    std::unique_ptr<TypeNode> inner_;
    bool                      isMut_;
};

// ─── SliceTypeNode ────────────────────────────────────────────────────────────

/// A dynamically-sized slice, e.g. `[T]`.
class SliceTypeNode : public TypeNode {
public:
    SliceTypeNode(std::unique_ptr<TypeNode> element,
                  common::SourceRange       range = {})
        : TypeNode(range), element_(std::move(element)) {}

    [[nodiscard]] const TypeNode& element() const noexcept { return *element_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "SliceTypeNode"; }

private:
    std::unique_ptr<TypeNode> element_;
};

// ─── ArrayTypeNode ────────────────────────────────────────────────────────────

/// A fixed-size array, e.g. `[T; N]`.
class ArrayTypeNode : public TypeNode {
public:
    ArrayTypeNode(std::unique_ptr<TypeNode> element,
                  std::unique_ptr<Expr>    size,
                  common::SourceRange      range = {})
        : TypeNode(range),
          element_(std::move(element)),
          size_(std::move(size)) {}

    [[nodiscard]] const TypeNode& element() const noexcept { return *element_; }
    [[nodiscard]] const Expr&     size()    const noexcept { return *size_; }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "ArrayTypeNode"; }

private:
    std::unique_ptr<TypeNode> element_;
    std::unique_ptr<Expr>     size_;
};

// ─── FunctionTypeNode ─────────────────────────────────────────────────────────

/// A function type: `fn(i32, bool) -> str`.
class FunctionTypeNode : public TypeNode {
public:
    FunctionTypeNode(std::vector<std::unique_ptr<TypeNode>> params,
                     std::unique_ptr<TypeNode>              returnType,
                     common::SourceRange                    range = {})
        : TypeNode(range),
          params_(std::move(params)),
          returnType_(std::move(returnType)) {}

    [[nodiscard]] const std::vector<std::unique_ptr<TypeNode>>& params()     const noexcept { return params_; }
    [[nodiscard]] const TypeNode*                                returnType() const noexcept { return returnType_.get(); }

    void accept(ASTVisitor& v) override { v.visit(*this); }
    [[nodiscard]] std::string_view nodeName() const noexcept override { return "FunctionTypeNode"; }

private:
    std::vector<std::unique_ptr<TypeNode>> params_;
    std::unique_ptr<TypeNode>              returnType_;  ///< nullptr = void/unit
};

} // namespace vcc::ast
