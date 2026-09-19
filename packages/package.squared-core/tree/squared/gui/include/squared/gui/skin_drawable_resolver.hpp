#pragma once

#include <squared/gui/drawable_ptr.hpp>

#include <functional>
#include <string_view>

namespace sq::gui {

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

} // namespace sq::gui
