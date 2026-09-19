#pragma once

#include <squared/gui/font_ptr.hpp>

#include <functional>
#include <string_view>

namespace sq::gui {

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

} // namespace sq::gui
