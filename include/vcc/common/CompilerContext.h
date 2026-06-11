#pragma once

#include "Diagnostics.h"
#include "SourceLocation.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/// @file CompilerContext.h
/// Central compilation context threaded through every compiler phase.
///
/// CompilerContext owns
///  • The set of registered source files (path + text buffer).
///  • The DiagnosticsEngine for this compilation.
///  • The CompilerOptions parsed from the CLI.
///
/// Every compiler phase receives a reference to CompilerContext so that
/// diagnostics, file lookups, and option queries share a single object.
///
/// Future extensions
/// ─────────────────
///  • String-intern table for identifier deduplication.
///  • Target-triple and data-layout information.
///  • Built-in types registry.
///  • Plugin / pass manager hook.

namespace vcc::common {

// ─── Options ─────────────────────────────────────────────────────────────────

/// Command-line options for a single compiler invocation.
struct CompilerOptions {
    std::vector<std::filesystem::path> inputFiles;
    std::filesystem::path              outputFile{"a.out"};

    // ── Debug / inspection flags ──────────────────────────────────────────
    bool dumpTokens{false};  ///< Print the token stream and exit.
    bool dumpAST   {false};  ///< Print the AST and exit.
    bool dumpIR    {false};  ///< Print VCC IR and exit.
    bool emitLLVM  {false};  ///< Lower to LLVM IR and write a .ll file.
    bool noCodegen {false};  ///< Stop after semantic analysis (syntax check).

    // ── Optimisation ─────────────────────────────────────────────────────
    int  optimizationLevel{0};  ///< 0 = none, 1–3 = increasing aggression

    // ── Verbosity ────────────────────────────────────────────────────────
    bool verbose{false};

    // ── Diagnostic display ────────────────────────────────────────────────
    bool noColor{false};  ///< Disable ANSI colour in diagnostics (--no-color).
};

// ─── Context ─────────────────────────────────────────────────────────────────

class CompilerContext {
public:
    explicit CompilerContext(CompilerOptions opts = {});

    // Non-copyable; move-only to prevent accidental duplication.
    CompilerContext(const CompilerContext&)            = delete;
    CompilerContext& operator=(const CompilerContext&) = delete;
    CompilerContext(CompilerContext&&)                 = default;
    CompilerContext& operator=(CompilerContext&&)      = default;

    ~CompilerContext() = default;

    // ── Options ───────────────────────────────────────────────────────────
    [[nodiscard]] CompilerOptions&       options()       noexcept;
    [[nodiscard]] const CompilerOptions& options() const noexcept;

    // ── Diagnostics ───────────────────────────────────────────────────────
    [[nodiscard]] DiagnosticsEngine&       diagnostics()       noexcept;
    [[nodiscard]] const DiagnosticsEngine& diagnostics() const noexcept;

    // ── Source file registry ──────────────────────────────────────────────

    /// Register a source file; returns its FileID (1-based).
    /// The context takes ownership of the text buffer.
    [[nodiscard]] FileID addSource(std::filesystem::path path, std::string text);

    /// Text of the registered source; empty view if @p id is unknown.
    [[nodiscard]] std::string_view sourceText(FileID id) const noexcept;

    /// Filesystem path of the registered source.
    [[nodiscard]] std::string_view sourcePath(FileID id) const noexcept;

    /// Number of registered source files.
    [[nodiscard]] std::size_t sourceCount() const noexcept;

private:
    struct SourceEntry {
        std::filesystem::path path;
        std::string           text;
    };

    CompilerOptions          opts_;
    DiagnosticsEngine        diag_;
    std::vector<SourceEntry> sources_;  ///< Indexed by (FileID - 1)
};

} // namespace vcc::common
