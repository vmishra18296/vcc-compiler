#include "vcc/common/CompilerContext.h"

#include <stdexcept>

namespace vcc::common {

CompilerContext::CompilerContext(CompilerOptions opts)
    : opts_(std::move(opts)) {}

CompilerOptions&       CompilerContext::options()       noexcept { return opts_; }
const CompilerOptions& CompilerContext::options() const noexcept { return opts_; }

DiagnosticsEngine&       CompilerContext::diagnostics()       noexcept { return diag_; }
const DiagnosticsEngine& CompilerContext::diagnostics() const noexcept { return diag_; }

FileID CompilerContext::addSource(std::filesystem::path path, std::string text) {
    sources_.push_back({std::move(path), std::move(text)});
    // FileID is 1-based so callers can use 0 as "invalid".
    return static_cast<FileID>(sources_.size());
}

std::string_view CompilerContext::sourceText(FileID id) const noexcept {
    if (id == 0 || id > static_cast<FileID>(sources_.size())) return {};
    return sources_[id - 1].text;
}

std::string_view CompilerContext::sourcePath(FileID id) const noexcept {
    if (id == 0 || id > static_cast<FileID>(sources_.size())) return {};
    return sources_[id - 1].path.native();
}

std::size_t CompilerContext::sourceCount() const noexcept {
    return sources_.size();
}

} // namespace vcc::common
