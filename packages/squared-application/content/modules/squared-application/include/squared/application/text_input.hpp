#pragma once

namespace squared::application {

/**
 * @brief Desired soft-keyboard content mode.
 *
 * @sa TextInputRequest::purpose
 */
enum class TextInputPurpose { normal, number, email, password };

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

} // namespace squared::application