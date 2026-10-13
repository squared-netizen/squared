#pragma once

#include <cstddef>

namespace sq::gui {

/**
 * @brief Explicit resource limits applied to one transactional skin load.
 */
struct SkinLoadLimits final {
    /** @brief Maximum accepted JSON document size in bytes. */
    std::size_t maximum_json_bytes{1024U * 1024U};

    /** @brief Maximum accepted JSON nesting depth. */
    std::size_t maximum_depth{64};

    /** @brief Maximum accepted number of loaded resources. */
    std::size_t maximum_resources{4096};

    /** @brief Maximum accepted length of one resource name in bytes. */
    std::size_t maximum_name_bytes{128};
};

} // namespace sq::gui
