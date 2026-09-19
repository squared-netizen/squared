#pragma once

#include <squared/gui/skin_load_severity.hpp>

#include <string>

namespace sq::gui {

/**
 * @brief One path-tagged diagnostics entry produced by skin loading.
 */
struct SkinLoadIssue final {
    /** @brief Severity of the issue. */
    SkinLoadSeverity severity{SkinLoadSeverity::error};

    /** @brief JSON pointer or member path at which the issue occurred. */
    std::string path;

    /** @brief Human-readable diagnostics message. */
    std::string message;
};

} // namespace sq::gui
