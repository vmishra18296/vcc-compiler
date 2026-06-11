#pragma once

#include <cstdint>
#include <string_view>

/// @file SourceLocation.h
/// Lightweight value types that encode positions inside V source files.
/// Every token, AST node and diagnostic carries one of these so that error
/// messages can point to the exact character that caused the problem.
///
/// Future: integrate with a SourceManager that maps FileIDs to memory-mapped
/// buffers and supports macro-expansion source ranges.

namespace vcc::common {

/// Opaque identifier for a source file registered with CompilerContext.
/// 0 is reserved as "invalid / unknown file".
using FileID = uint32_t;

/// Encodes a single character position within a source file.
/// Cheap to copy; stored inline in Token and every ASTNode.
struct SourceLocation {
    FileID   file{0};
    uint32_t line{0};    ///< 1-based line number
    uint32_t column{0};  ///< 1-based UTF-8 column number

    /// Returns true iff the location refers to an actual position.
    [[nodiscard]] constexpr bool isValid() const noexcept { return line != 0; }

    /// Sentinel for "no location available".
    [[nodiscard]] static constexpr SourceLocation invalid() noexcept { return {}; }

    // Comparison (needed for diagnostics deduplication, tests, etc.)
    constexpr bool operator==(const SourceLocation&) const noexcept = default;
    constexpr bool operator!=(const SourceLocation&) const noexcept = default;
};

/// Half-open character range [begin, end) within a single source file.
/// Used by AST nodes that span multiple tokens.
///
/// Future: support multi-file ranges once the macro expander is in place.
struct SourceRange {
    SourceLocation begin;
    SourceLocation end;

    [[nodiscard]] constexpr bool isValid() const noexcept {
        return begin.isValid() && end.isValid();
    }

    /// Convenience factory — spans from one location to another.
    [[nodiscard]] static constexpr SourceRange
    span(SourceLocation b, SourceLocation e) noexcept {
        return {b, e};
    }

    /// Single-character range.
    [[nodiscard]] static constexpr SourceRange
    at(SourceLocation loc) noexcept {
        return {loc, loc};
    }
};

} // namespace vcc::common
