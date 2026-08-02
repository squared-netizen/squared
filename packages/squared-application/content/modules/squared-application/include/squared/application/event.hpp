#pragma once

#include <cstdint>

namespace squared::application {

/**
 * @brief Platform-neutral application event.
 *
 * The SDL platform adapter converts supported SDL events into this compact
 * representation before developer application code receives them.
 */
struct Event final {
    /** @brief Supported event categories for the Phase 5 boundary. */
    enum class Type {
        QuitRequested,
        PointerDown,
        PointerMove,
        PointerUp,
        BackRequested,
        Pause,
        Resume,
        Resize
    };

    Type type{Type::QuitRequested};
    std::int64_t pointer_id{0};
    float x{0.0F};
    float y{0.0F};
    int width{0};
    int height{0};
};

}  // namespace squared::application
