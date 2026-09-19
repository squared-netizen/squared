#pragma once

// Aggregate header for the skin_loader group of
// sq::gui. Include a single type's header
// directly to keep a translation unit narrow.

#include <squared/gui/gui.hpp>
#include <squared/gui/skin_drawable_resolver.hpp>
#include <squared/gui/skin_font_resolver.hpp>
#include <squared/gui/skin_load_issue.hpp>
#include <squared/gui/skin_load_limits.hpp>
#include <squared/gui/skin_load_report.hpp>
#include <squared/gui/skin_load_severity.hpp>
#include <squared/graphics2d/texture_atlas.hpp>
#include <squared/gui/drawable_ptr.hpp>
#include <squared/gui/skin.hpp>

#include <string_view>

namespace sq::gui {

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

} // namespace sq::gui
