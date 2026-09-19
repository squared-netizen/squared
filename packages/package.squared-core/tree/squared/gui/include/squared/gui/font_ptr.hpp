#pragma once

#include <squared/gui/font_resource.hpp>

#include <memory>

namespace sq::gui {

/** @brief Immutable shared ownership of one GUI font resource. */
using FontPtr = std::shared_ptr<const FontResource>;

} // namespace sq::gui
