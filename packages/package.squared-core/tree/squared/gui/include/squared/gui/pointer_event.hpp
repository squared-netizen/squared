#pragma once

#include <squared/gui/pointer_action.hpp>

#include <cstdint>

namespace sq::gui {

/** @brief Portable pointer event payload in widget-local logical units. */
struct PointerEvent {
    /** @brief Action being reported. */
    PointerAction action{PointerAction::move};

    /** @brief Stable identifier for the pointer/contact. */
    std::int64_t pointer_id{0};

    /** @brief Horizontal position in logical pixels. */
    float x{0.0F};

    /** @brief Vertical position in logical pixels. */
    float y{0.0F};

    /** @brief Button index; zero means no button or the primary button. */
    int button{0};
};

} // namespace sq::gui
