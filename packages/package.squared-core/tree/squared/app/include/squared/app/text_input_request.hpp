#pragma once

#include <squared/app/text_input_area.hpp>
#include <squared/app/text_input_purpose.hpp>

namespace sq::app {

/**
 * @brief Request sent when platform text input starts.
 *
 * @param area The current on-screen target rectangle.
 * @param purpose The keyboard layout the platform should present.
 */
struct TextInputRequest {
    /** @brief Current on-screen target rectangle. */
    TextInputArea area{};
    /** @brief Requested soft-keyboard content mode. */
    TextInputPurpose purpose{TextInputPurpose::normal};
};

}  // namespace sq::app
