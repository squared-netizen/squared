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
 * @brief Resolve one declared bitmap-font resource without fixing an I/O API.
 *
 * The callback may return a resolved FontResource with parsed Graphics2D
 * metrics and page regions, or a descriptor-only resource. Every texture
 * referenced by a resolved result must outlive the destination Skin.
 * @param resource_name Name declared in the libGDX font section.
 * @param descriptor_path Safe relative descriptor path from that declaration.
 * @return Immutable font resource, or empty to reject the skin.
 */
using SkinFontResolver = std::function<FontPtr(
    std::string_view resource_name,
    std::string_view descriptor_path
)>;

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
 * @param font_resolver Callback resolving declared bitmap-font resources.
 * @param report Diagnostics and counted resources collected during loading.
 * @param limits Explicit resource limits applied to the document.
 * @return true when the load committed; false when any error occurred.
 * @note Threading: caller must own both the Skin and the resolver.
 */
[[nodiscard]] bool load_libgdx_skin(
    Skin& destination,
    std::string_view json,
    const SkinDrawableResolver& resolver,
    const SkinFontResolver& font_resolver,
    SkinLoadReport& report,
    const SkinLoadLimits& limits = {}
) noexcept;

/**
 * @brief Load a skin while retaining fonts as descriptor-only resources.
 *
 * This compatibility overload preserves existing Painter implementations.
 * Font-aware applications should provide SkinFontResolver through the full
 * overload.
 * @param destination Skin replaced only after a successful complete import.
 * @param json Complete relaxed libGDX skin JSON document.
 * @param resolver Callback resolving drawable resource names.
 * @param report Diagnostics and resource counts, replaced for this call.
 * @param limits Explicit byte, depth, resource, and name bounds.
 * @return true when the skin committed; false with destination unchanged.
 */
[[nodiscard]] bool load_libgdx_skin(
    Skin& destination,
    std::string_view json,
    const SkinDrawableResolver& resolver,
    SkinLoadReport& report,
    const SkinLoadLimits& limits = {}
) noexcept;

} // namespace squared::gui
