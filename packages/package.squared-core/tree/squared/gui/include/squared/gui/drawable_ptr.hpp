#pragma once

#include <squared/gui/drawable.hpp>

#include <memory>

namespace sq::gui {

/**
 * @brief Shared ownership of an immutable drawable.
 */
using DrawablePtr = std::shared_ptr<const Drawable>;

} // namespace sq::gui
