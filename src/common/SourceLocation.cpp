#include "vcc/common/SourceLocation.h"

// SourceLocation and SourceRange are pure value types defined in the header.
// This translation unit exists so that the vcc_common library target is never
// empty, and to house any future utility functions (e.g., pretty-printing
// locations given a SourceManager) without changing the header.

namespace vcc::common {
// Future: SourceManager helper functions (line/column lookup, file name cache)
// will be implemented here once the SourceManager class is added in Phase 2.
} // namespace vcc::common
