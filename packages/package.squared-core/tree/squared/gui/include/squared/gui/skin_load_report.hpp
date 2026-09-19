#pragma once

#include <squared/gui/skin_load_issue.hpp>

#include <cstddef>
#include <vector>

namespace sq::gui {

/**
 * @brief Counted result of one transactional skin load.
 */
struct SkinLoadReport final {
    /** @brief Diagnostics collected during loading, in order. */
    std::vector<SkinLoadIssue> issues;

    /** @brief Number of colors accepted into the destination skin. */
    std::size_t colors_loaded{0};

    /** @brief Number of drawables accepted into the destination skin. */
    std::size_t drawables_loaded{0};

    /** @brief Number of named font resources accepted into the skin. */
    std::size_t fonts_loaded{0};

    /** @brief Number of widget styles accepted into the destination skin. */
    std::size_t styles_loaded{0};

    /**
     * @brief Report whether the load committed successfully.
     * @return true when no error-severity issue was recorded.
     */
    [[nodiscard]] bool success() const noexcept;
};

} // namespace sq::gui
