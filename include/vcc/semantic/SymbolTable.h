#pragma once

#include "vcc/common/SourceLocation.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/// @file SymbolTable.h
/// Scope-aware symbol table used during semantic analysis.
///
/// Design
/// ──────
///  • Scopes form a stack: entering a block pushes, leaving pops.
///  • Symbol lookup walks from the innermost to the outermost scope.
///  • The table is intentionally decoupled from the AST so it can be used
///    by both the semantic analyser and the IR generator.
///
/// Future extensions
/// ─────────────────
///  • Attach resolved type information (a `Type*` from the type system).
///  • Support for generic symbols and their instantiation.
///  • Module-level symbol tables linked for multi-file compilation.
///  • Overload sets for functions with the same name.

namespace vcc::semantic {

// ─── Symbol ───────────────────────────────────────────────────────────────────

enum class SymbolKind {
    Variable,    ///< let / var binding
    Constant,    ///< const binding
    Function,    ///< fn declaration
    Parameter,   ///< function parameter
    Type,        ///< struct / enum / type alias
    Module,      ///< imported module
};

[[nodiscard]] std::string_view symbolKindName(SymbolKind kind) noexcept;

struct Symbol {
    std::string name;
    SymbolKind  kind;
    bool        isMut{false};  ///< mutable variable?
    bool        isPub{false};  ///< public declaration?

    /// Future: replace with a pointer into the type-system.
    std::string typeName;

    /// Source location encoded as "file:line:col" for diagnostics.
    std::string declarationSite;

    /// Precise source location — used to emit "declared here" notes.
    common::SourceLocation declLoc;

    /// Function-specific metadata (only populated when kind == Function).
    std::vector<std::string> paramTypes;  ///< Ordered parameter type names.
    std::string              returnType;  ///< Return type (empty = unit/void).
};

// ─── Typed symbol helpers ─────────────────────────────────────────────────────

/// A variable binding (let / var).  Convenience subtype of Symbol.
struct VariableSymbol : Symbol {
    VariableSymbol() = default;
    VariableSymbol(std::string nm, std::string type, bool mut = false,
                   bool pub = false) {
        name      = std::move(nm);
        typeName  = std::move(type);
        kind      = mut ? SymbolKind::Variable : SymbolKind::Variable;
        isMut     = mut;
        isPub     = pub;
    }
};

/// A function declaration.  Convenience constructor that sets all fields.
struct FunctionSymbol : Symbol {
    FunctionSymbol() = default;
    FunctionSymbol(std::string nm,
                   std::vector<std::string> params,
                   std::string              ret,
                   bool                     pub = false) {
        name        = std::move(nm);
        kind        = SymbolKind::Function;
        paramTypes  = std::move(params);
        returnType  = std::move(ret);
        isPub       = pub;
        typeName    = returnType;
    }
};

// ─── Scope ────────────────────────────────────────────────────────────────────

class Scope {
public:
    explicit Scope(std::string label = {}) : label_(std::move(label)) {}

    /// Returns true if a symbol with this name was inserted; false if it already
    /// existed in this scope (redefinition error).
    bool define(Symbol sym);

    /// Returns a pointer to the Symbol or nullptr if not found in this scope.
    [[nodiscard]] const Symbol* lookup(const std::string& name) const noexcept;

    [[nodiscard]] const std::string& label() const noexcept { return label_; }

private:
    std::string                          label_;
    std::unordered_map<std::string, Symbol> symbols_;
};

// ─── SymbolTable ──────────────────────────────────────────────────────────────

/// A stack of scopes.  Always has at least one (global) scope.
class SymbolTable {
public:
    SymbolTable();

    /// Push a new nested scope (e.g., entering a function body or if-block).
    void enterScope(std::string label = {});

    /// Pop the current scope.  Asserts that the global scope is never popped.
    void leaveScope();

    /// Alias for leaveScope() – matches the requirement-spec method name.
    void exitScope() { leaveScope(); }

    /// Define a symbol in the current (innermost) scope.
    /// @returns false if the name is already defined in the current scope.
    bool define(Symbol sym);

    /// Look up a name, walking from innermost to outermost scope.
    /// @returns nullptr if not found.
    [[nodiscard]] const Symbol* lookup(const std::string& name) const noexcept;

    /// Look up in the current scope only (for redefinition checks).
    [[nodiscard]] const Symbol* lookupCurrent(const std::string& name) const noexcept;

    /// Current nesting depth (0 = global scope).
    [[nodiscard]] std::size_t depth() const noexcept;

private:
    std::vector<Scope> scopes_;
};

} // namespace vcc::semantic
