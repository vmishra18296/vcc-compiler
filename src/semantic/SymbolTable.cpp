#include "vcc/semantic/SymbolTable.h"

#include <cassert>

namespace vcc::semantic {

// ─── Free helpers ─────────────────────────────────────────────────────────────

std::string_view symbolKindName(SymbolKind kind) noexcept {
    switch (kind) {
        case SymbolKind::Variable:  return "variable";
        case SymbolKind::Constant:  return "constant";
        case SymbolKind::Function:  return "function";
        case SymbolKind::Parameter: return "parameter";
        case SymbolKind::Type:      return "type";
        case SymbolKind::Module:    return "module";
    }
    return "unknown";
}

// ─── Scope ────────────────────────────────────────────────────────────────────

bool Scope::define(Symbol sym) {
    const auto [it, inserted] = symbols_.emplace(sym.name, std::move(sym));
    return inserted;
}

const Symbol* Scope::lookup(const std::string& name) const noexcept {
    const auto it = symbols_.find(name);
    return (it != symbols_.end()) ? &it->second : nullptr;
}

// ─── SymbolTable ──────────────────────────────────────────────────────────────

SymbolTable::SymbolTable() {
    // Ensure there is always a global scope.
    scopes_.emplace_back("global");
}

void SymbolTable::enterScope(std::string label) {
    scopes_.emplace_back(std::move(label));
}

void SymbolTable::leaveScope() {
    assert(scopes_.size() > 1 && "Cannot pop the global scope");
    scopes_.pop_back();
}

bool SymbolTable::define(Symbol sym) {
    return scopes_.back().define(std::move(sym));
}

const Symbol* SymbolTable::lookup(const std::string& name) const noexcept {
    // Walk from innermost scope outward.
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        if (const Symbol* s = it->lookup(name)) return s;
    }
    return nullptr;
}

const Symbol* SymbolTable::lookupCurrent(const std::string& name) const noexcept {
    return scopes_.back().lookup(name);
}

std::size_t SymbolTable::depth() const noexcept {
    return scopes_.size() - 1;  // global = depth 0
}

} // namespace vcc::semantic
