#pragma once

#include <squared/gui/size.hpp>

#include <limits>

namespace sq::gui {

/** @brief Proposed minimum, preferred, and maximum sizes for one widget. */
struct SizeHints {
    /** @brief Smallest acceptable extent in logical units. */
    Size minimum{};
    /** @brief Extent the widget occupies when unconstrained. */
    Size preferred{};
    /** @brief Largest acceptable extent; infinity means unconstrained. */
    Size maximum{
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity()
    };
};

} // namespace sq::gui
