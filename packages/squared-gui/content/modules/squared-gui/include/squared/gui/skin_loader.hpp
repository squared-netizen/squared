#pragma once

#include <squared/gui/gui.hpp>

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace squared::gui {

/**
 * @brief Severity of one skin-loading diagnostics entry.
 */
enum class SkinLoadSeverity { warning, error };

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

    /** @brief Number of widget styles accepted into the destination skin. */
    std::size_t styles_loaded{0};

    /**
     * @brief Report whether the load committed successfully.
     * @return true when no error-severity issue was recorded.
     */
    [[nodiscard]] bool success() const noexcept;
};

/**
 * @brief Resolves one atlas/resource name without exposing storage or backend APIs.
 *
 * The returned drawable and every texture it references must outlive the Skin.
 * @param resource_name Name of the drawable/resource to resolve.
 * @return Drawable for the requested name, or an empty pointer when
 * resolution failed.
 */
using SkinDrawableResolver =
    std::function<DrawablePtr(std::string_view resource_name)>;

/**
 * @brief Create a region or nine-patch drawable for one loaded atlas region.
 * @param atlas Atlas owning the requested region; must outlive the result.
 * @param resource_name Name and optional index of the atlas region.
 * @return Drawable referencing the atlas region, or an empty pointer when
 * the region is absent or cannot be converted.
 */
[[nodiscard]] DrawablePtr resolve_atlas_drawable(
    const graphics2d::TextureAtlas& atlas,
    std::string_view resource_name
);

/**
 * @brief Load the supported libGDX skin subset transactionally from memory.
 *
 * The relaxed libGDX JSON dialect is normalized under explicit limits. The
 * destination is replaced only when every supported style validates and every
 * referenced drawable resolves.
 * @param destination Skin receiving the loaded colors, drawables, and styles;
 * untouched byte-for-byte when the load fails.
 * @param json Complete skin JSON document in the relaxed libGDX dialect.
 * @param resolver Callback resolving every referenced resource name.
 * @param report Diagnostics and counted resources collected during loading.
 * @param limits Explicit resource limits applied to the document.
 * @return true when the load committed; false when any error occurred.
 * @note Threading: caller must own both the Skin and the resolver.
 */
[[nodiscard]] bool load_libgdx_skin(
    Skin& destination,
    std::string_view json,
    const SkinDrawableResolver& resolver,
    SkinLoadReport& report,
    const SkinLoadLimits& limits = {}
) noexcept;

} // namespace squared::gui
