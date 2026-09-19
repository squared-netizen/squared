#pragma once

#include <squared/app/text_input_area.hpp>
#include <squared/app/text_input_request.hpp>

namespace sq::app {

/**
 * @brief Portable application-to-platform soft-keyboard boundary.
 *
 * This interface is implemented by the platform adapter. Application and GUI
 * code obtains a pointer through Application::set_text_input_service and
 * never depends on SDL or Android keyboard APIs. All methods are called by
 * the widget framework on the application thread; the implementation owns
 * platform IPC.
 */
class TextInputService {
public:
    virtual ~TextInputService() = default;

    /**
     * @brief Show the soft keyboard for one focused control.
     * @param request The target rectangle and requested keyboard purpose.
     */
    virtual void start(const TextInputRequest& request) = 0;

    /**
     * @brief Reposition the soft keyboard when the control moves or resizes.
     * @param area The updated on-screen target rectangle.
     */
    virtual void update_area(const TextInputArea& area) = 0;

    /** @brief Hide the soft keyboard. Idempotent when already hidden. */
    virtual void stop() = 0;

    /**
     * @brief Report whether platform text input is active.
     * @return true while the soft keyboard is shown for a control.
     */
    [[nodiscard]] virtual bool active() const noexcept = 0;
};

}  // namespace sq::app
