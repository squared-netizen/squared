#pragma once

#include <cstdint>

namespace sq::app {

/**
 * @brief The edges of the drawable that something else covers.
 *
 * On a phone the application draws edge to edge, and the status bar, the
 * navigation bar and a camera cut-out sit on top of it. Each member is how far
 * that covering reaches in from one edge, in drawable pixels. All zero means
 * the whole drawable is usable, which is also what a desktop window reports.
 *
 * Eight bytes, passed by const reference. A value type with no behaviour: it
 * is measured by the platform layer and consumed by the application.
 */
struct SafeArea final {
    /** @brief Pixels covered along the left edge. */
    std::int16_t left{0};
    /** @brief Pixels covered along the top edge (the status bar). */
    std::int16_t top{0};
    /** @brief Pixels covered along the right edge. */
    std::int16_t right{0};
    /** @brief Pixels covered along the bottom edge (the navigation bar). */
    std::int16_t bottom{0};

    [[nodiscard]] friend constexpr bool operator==(const SafeArea&,
                                                   const SafeArea&) noexcept = default;
};

static_assert(sizeof(SafeArea) == 8, "SafeArea is four 16-bit pixel counts");

}  // namespace sq::app
