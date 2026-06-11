#include "vcc/common/Diagnostics.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace vcc::common {

// ─── ANSI escape sequences ────────────────────────────────────────────────────

namespace {

constexpr const char* kReset      = "\033[0m";
constexpr const char* kBold       = "\033[1m";
constexpr const char* kDim        = "\033[2m";
constexpr const char* kBoldRed    = "\033[1;31m";
constexpr const char* kBoldYellow = "\033[1;33m";
constexpr const char* kBoldCyan   = "\033[1;36m";
constexpr const char* kBoldGreen  = "\033[1;32m";
constexpr const char* kBoldWhite  = "\033[1;37m";

} // anonymous namespace

// ─── Free helpers ─────────────────────────────────────────────────────────────

std::string_view diagnosticLevelName(DiagnosticLevel lvl) noexcept {
    switch (lvl) {
        case DiagnosticLevel::Note:    return "note";
        case DiagnosticLevel::Warning: return "warning";
        case DiagnosticLevel::Error:   return "error";
        case DiagnosticLevel::Fatal:   return "fatal error";
    }
    return "unknown";
}

// ─── Diagnostic::format ──────────────────────────────────────────────────────

std::string Diagnostic::format(std::string_view filePath) const {
    std::ostringstream oss;
    // Format:  file:line:col: level: message
    if (!filePath.empty()) {
        oss << filePath << ':';
    } else if (location.isValid()) {
        oss << "<source>:";
    }
    if (location.isValid()) {
        oss << location.line << ':' << location.column << ": ";
    }
    oss << diagnosticLevelName(level) << ": " << message;
    if (!fixIt.empty()) {
        oss << "\n  hint: " << fixIt;
    }
    return oss.str();
}

// ─── DiagnosticsEngine ───────────────────────────────────────────────────────

DiagnosticsEngine::DiagnosticsEngine(Handler handler)
    : handler_(std::move(handler)) {}

void DiagnosticsEngine::emit(DiagnosticLevel level,
                              SourceLocation  loc,
                              std::string     msg) {
    Diagnostic d{level, loc, std::move(msg)};
    diagnostics_.push_back(d);

    switch (level) {
        case DiagnosticLevel::Warning: ++warningCount_; break;
        case DiagnosticLevel::Error:   ++errorCount_;   break;
        case DiagnosticLevel::Fatal:   ++errorCount_; hasFatal_ = true; break;
        default: break;
    }

    if (handler_) {
        handler_(diagnostics_.back());
    }
}

void DiagnosticsEngine::note   (SourceLocation loc, std::string msg) { emit(DiagnosticLevel::Note,    loc, std::move(msg)); }
void DiagnosticsEngine::warning(SourceLocation loc, std::string msg) { emit(DiagnosticLevel::Warning, loc, std::move(msg)); }
void DiagnosticsEngine::error  (SourceLocation loc, std::string msg) { emit(DiagnosticLevel::Error,   loc, std::move(msg)); }
void DiagnosticsEngine::fatal  (SourceLocation loc, std::string msg) { emit(DiagnosticLevel::Fatal,   loc, std::move(msg)); }

bool     DiagnosticsEngine::hasErrors()    const noexcept { return errorCount_ > 0; }
bool     DiagnosticsEngine::hasFatal()     const noexcept { return hasFatal_; }
uint32_t DiagnosticsEngine::errorCount()   const noexcept { return errorCount_; }
uint32_t DiagnosticsEngine::warningCount() const noexcept { return warningCount_; }

const std::vector<Diagnostic>& DiagnosticsEngine::all() const noexcept {
    return diagnostics_;
}

void DiagnosticsEngine::printAll(std::ostream& out, std::string_view filePath) const {
    for (const auto& d : diagnostics_) {
        out << d.format(filePath) << '\n';
    }
}

void DiagnosticsEngine::printAllPretty(std::ostream&          out,
                                        DiagnosticSourceLookup lookup,
                                        bool                   useColor) const {
    DiagnosticRenderer renderer(out, useColor, std::move(lookup));
    renderer.renderAll(diagnostics_);
}

void DiagnosticsEngine::reset() {
    diagnostics_.clear();
    errorCount_   = 0;
    warningCount_ = 0;
    hasFatal_     = false;
}

// ─── DiagnosticRenderer ───────────────────────────────────────────────────────

DiagnosticRenderer::DiagnosticRenderer(std::ostream&          out,
                                        bool                   useColor,
                                        DiagnosticSourceLookup lookup)
    : out_(out), useColor_(useColor), lookup_(std::move(lookup)) {}

// ── Helpers ───────────────────────────────────────────────────────────────────

const char* DiagnosticRenderer::levelColor(DiagnosticLevel lvl) const noexcept {
    if (!useColor_) return "";
    switch (lvl) {
        case DiagnosticLevel::Note:    return kBoldCyan;
        case DiagnosticLevel::Warning: return kBoldYellow;
        case DiagnosticLevel::Error:   return kBoldRed;
        case DiagnosticLevel::Fatal:   return kBoldRed;
    }
    return "";
}

std::string_view DiagnosticRenderer::extractLine(std::string_view source,
                                                  uint32_t         lineNumber) noexcept {
    uint32_t line = 1;
    size_t   pos  = 0;
    while (pos <= source.size()) {
        size_t nl = source.find('\n', pos);
        if (nl == std::string_view::npos) nl = source.size();
        if (line == lineNumber)
            return source.substr(pos, nl - pos);
        if (nl == source.size()) break;
        ++line;
        pos = nl + 1;
    }
    return {};
}

// ── render ────────────────────────────────────────────────────────────────────

void DiagnosticRenderer::render(const Diagnostic& d) const {
    const char* bold  = useColor_ ? kBold      : "";
    const char* dim   = useColor_ ? kDim       : "";
    const char* reset = useColor_ ? kReset     : "";
    const char* green = useColor_ ? kBoldGreen : "";
    const char* white = useColor_ ? kBoldWhite : "";

    // ── 1. Header: file:line:col: level: message ──────────────────────────────

    std::string_view filePath   = "<source>";
    std::string_view sourceText;

    if (lookup_ && d.location.file != 0) {
        auto [src, path] = lookup_(d.location.file);
        sourceText = src;
        if (!path.empty()) filePath = path;
    }

    // Bold file/location prefix
    out_ << white;
    if (d.location.isValid()) {
        out_ << filePath << ':' << d.location.line << ':' << d.location.column << ':';
    } else if (filePath != "<source>") {
        out_ << filePath << ':';
    }
    out_ << reset << ' ';

    // Coloured level label
    out_ << levelColor(d.level) << diagnosticLevelName(d.level) << ':' << reset;

    // Bold message text
    out_ << ' ' << bold << d.message << reset << '\n';

    // ── 2. Source context panel ───────────────────────────────────────────────

    if (!sourceText.empty() && d.location.isValid()) {
        const std::string_view lineText = extractLine(sourceText, d.location.line);

        // Compute gutter width: right-justify the line number.
        const std::string lineNumStr = std::to_string(d.location.line);
        const int gutterW = static_cast<int>(std::max<size_t>(lineNumStr.size(), 4u));
        const std::string spaces(gutterW, ' ');

        // Source line: "  12 │ <text>"
        out_ << dim;
        out_ << std::setw(gutterW) << std::right << lineNumStr;
        out_ << reset;
        out_ << " \u2502 " << lineText << '\n';

        // Caret line: "     │ <spaces>^~~~"
        out_ << dim << spaces << reset;
        out_ << " \u2502 ";

        // Advance to column (1-based → 0-based offset).
        const uint32_t col = (d.location.column > 0) ? d.location.column - 1 : 0;

        // Preserve any leading tab characters from the source line for alignment.
        for (uint32_t i = 0; i < col && i < lineText.size(); ++i) {
            out_ << (lineText[i] == '\t' ? '\t' : ' ');
        }
        // Emit caret and optional underline tildes.
        out_ << green << '^';
        const uint32_t underlineLen = (d.tokenLength > 1) ? d.tokenLength - 1 : 0;
        for (uint32_t i = 0; i < underlineLen; ++i) out_ << '~';
        out_ << reset << '\n';
    }

    // ── 3. Fix-it hint ────────────────────────────────────────────────────────

    if (!d.fixIt.empty()) {
        out_ << "  hint: " << d.fixIt << '\n';
    }
}

void DiagnosticRenderer::renderAll(const std::vector<Diagnostic>& diags) const {
    for (const auto& d : diags) {
        render(d);
    }
}

} // namespace vcc::common
