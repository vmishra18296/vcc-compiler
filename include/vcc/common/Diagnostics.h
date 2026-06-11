#pragma once

#include "SourceLocation.h"

#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/// @file Diagnostics.h
/// Compiler diagnostic infrastructure for VCC.
///
/// Design goals
/// ────────────
///  • Every diagnostic is associated with a SourceLocation.
///  • Callers emit via DiagnosticsEngine; the engine decides how to display/
///    route messages (stderr, IDE protocol, JSON, …).
///  • Fatal diagnostics abort the current compilation phase.
///
/// Future extensions
/// ─────────────────
///  • Structured fix-it edits (insertions / replacements / removals).
///  • Secondary source ranges ("note: declared here").
///  • Machine-readable JSON output for IDE integration.
///  • Diagnostic suppression / upgrade rules (treat warnings as errors, etc.)

namespace vcc::common {

// ─── Severity ────────────────────────────────────────────────────────────────

enum class DiagnosticLevel : uint8_t {
    Note    = 0,  ///< Informational; never increments the error count.
    Warning = 1,  ///< Recoverable problem; controlled by -W flags.
    Error   = 2,  ///< Non-fatal; compilation continues to collect more errors.
    Fatal   = 3,  ///< Unrecoverable; compilation halts after this phase.
};

[[nodiscard]] std::string_view diagnosticLevelName(DiagnosticLevel lvl) noexcept;

// ─── Diagnostic value type ───────────────────────────────────────────────────

/// A single immutable diagnostic message.
struct Diagnostic {
    DiagnosticLevel level;
    SourceLocation  location;
    std::string     message;

    /// Optional one-line hint shown below the caret (future: structured edits).
    std::string fixIt;

    /// Length of the highlighted token in source (1 = caret only, N = caret + N-1 tildes).
    uint32_t tokenLength{1};

    /// Human-readable plain-text rendering for tests and non-TTY output.
    [[nodiscard]] std::string format(std::string_view filePath = "") const;
};

/// Callback that maps a FileID to its {source_text, file_path}.
/// Used by DiagnosticRenderer to show source context in error messages.
using DiagnosticSourceLookup =
    std::function<std::pair<std::string_view, std::string_view>(FileID)>;

// ─── Engine ──────────────────────────────────────────────────────────────────

/// Central hub for emitting, routing, and counting compiler diagnostics.
///
/// Usage:
/// @code
///   engine.error(loc, "undeclared identifier '" + name + "'");
/// @endcode
class DiagnosticsEngine {
public:
    /// Optional callback invoked for every emitted diagnostic.
    /// Useful for IDE/LSP integrations that need real-time feedback.
    using Handler = std::function<void(const Diagnostic&)>;

    explicit DiagnosticsEngine(Handler handler = nullptr);

    // ── Emission helpers ──────────────────────────────────────────────────────
    void note   (SourceLocation loc, std::string msg);
    void warning(SourceLocation loc, std::string msg);
    void error  (SourceLocation loc, std::string msg);

    /// Emits a fatal diagnostic.
    /// Callers should check hasFatal() and abort the current phase afterward.
    void fatal  (SourceLocation loc, std::string msg);

    // ── Query ─────────────────────────────────────────────────────────────────
    [[nodiscard]] bool     hasErrors()    const noexcept;
    [[nodiscard]] bool     hasFatal()     const noexcept;
    [[nodiscard]] uint32_t errorCount()   const noexcept;
    [[nodiscard]] uint32_t warningCount() const noexcept;

    /// Read-only access to all collected diagnostics (useful in tests).
    [[nodiscard]] const std::vector<Diagnostic>& all() const noexcept;

    /// Print every diagnostic to @p out in plain-text form (no ANSI codes).
    void printAll(std::ostream& out, std::string_view filePath = "") const;

    /// Pretty-print every diagnostic with optional ANSI colours and source
    /// context (source line + caret).  Pass a DiagnosticSourceLookup to enable
    /// the source-context panel; pass nullptr to omit it.
    ///
    /// @param out      Destination stream.
    /// @param lookup   Maps FileID → {source_text, file_path}; may be null.
    /// @param useColor Emit ANSI escape sequences (auto-detect via isatty()).
    void printAllPretty(std::ostream&                out,
                        DiagnosticSourceLookup       lookup  = nullptr,
                        bool                         useColor = true) const;

    /// Reset counters and clear collected diagnostics.
    void reset();

private:
    void emit(DiagnosticLevel level, SourceLocation loc, std::string msg);

    std::vector<Diagnostic> diagnostics_;
    Handler                 handler_;
    uint32_t                errorCount_{0};
    uint32_t                warningCount_{0};
    bool                    hasFatal_{false};
};

// ─── DiagnosticRenderer ──────────────────────────────────────────────────────

/// Renders compiler diagnostics to a stream in a Clang-inspired format:
///
/// @code
///   main.v:12:5: error: undefined variable 'count'
///      12 │     let x = count + 1
///         │             ^
/// @endcode
///
/// When @p useColor is true, ANSI escape sequences are emitted:
///   - Bold + red for errors and fatal diagnostics
///   - Bold + yellow for warnings
///   - Bold + cyan for notes
///   - Bold white for the file/location header
///   - Dim for the line-number gutter
///   - Bold green for the caret and underline
class DiagnosticRenderer {
public:
    /// @param out      Destination stream.
    /// @param useColor Emit ANSI escape sequences.
    /// @param lookup   Maps FileID → {source_text, file_path}; may be null.
    DiagnosticRenderer(std::ostream&          out,
                       bool                   useColor = true,
                       DiagnosticSourceLookup lookup   = nullptr);

    /// Render a single diagnostic (header + optional source-context panel).
    void render(const Diagnostic& d) const;

    /// Render every diagnostic in order.
    void renderAll(const std::vector<Diagnostic>& diags) const;

private:
    /// Return the bold ANSI colour string appropriate for @p lvl (or "" if
    /// colour is disabled).
    [[nodiscard]] const char* levelColor(DiagnosticLevel lvl) const noexcept;

    /// Extract the 1-based @p lineNumber from @p source.
    static std::string_view extractLine(std::string_view source,
                                        uint32_t         lineNumber) noexcept;

    std::ostream&          out_;
    bool                   useColor_;
    DiagnosticSourceLookup lookup_;
};

} // namespace vcc::common
