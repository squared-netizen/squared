#pragma once

namespace sq::app {

/**
 * @brief Soft-keyboard target rectangle in stage/logical coordinates.
 *
 * Units are logical pixels in the same coordinate space the application uses
 * for rendering. The platform adapter positions its on-screen keyboard to
 * cover or avoid this rectangle.
 */
struct TextInputArea {
    /** @brief Left edge in logical pixels. */
    float x{0};
    /** @brief Top edge in logical pixels. */
    float y{0};
    /** @brief Width in logical pixels; must be non-negative. */
    float width{0};
    /** @brief Height in logical pixels; must be non-negative. */
    float height{0};
};

}  // namespace sq::app
