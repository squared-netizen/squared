#pragma once

#include <cstdint>
#include <functional>

namespace squared::scene2d {

class Actor;

/** @brief Event kind carried by one InputEvent. */
enum class InputType {
    pointer_move,
    pointer_down,
    pointer_up,
    pointer_cancel,
    key_down,
    key_up,
    navigation
};

/** @brief Portable hardware-independent key names for key events. */
enum class InputKey {
    unknown,
    left,
    right,
    up,
    down,
    home,
    end,
    backspace,
    delete_key,
    enter,
    space,
    tab,
    escape
};

/** @brief Semantic focus and navigation action from any supported device. */
enum class NavigationAction {
    unknown,
    left,
    right,
    up,
    down,
    next,
    previous,
    activate,
    cancel
};

/** @brief Dispatch phase of an event inside a Stage. */
enum class InputPhase { capture, target, bubble };

/** @brief Backend-neutral modifier state copied at the input boundary. */
struct InputModifiers final {
    /** @brief Shift key pressed. */
    bool shift{false};
    /** @brief Control key pressed. */
    bool control{false};
    /** @brief Alt key pressed. */
    bool alt{false};
    /** @brief Meta key pressed. */
    bool meta{false};
};

/**
 * @brief Mutable event routed through a Stage actor path.
 *
 * Source fields are supplied by the caller. Stage supplies target, current
 * actor, phase, and actor-local pointer coordinates during dispatch.
 */
class InputEvent final {
public:
    /** @brief Event kind; supplied by the dispatcher or the caller. */
    InputType type{InputType::pointer_move};

    /** @brief Stable identifier of the pointer that produced the event. */
    std::int64_t pointer_id{0};

    /** @brief Horizontal position in stage logical coordinates. */
    float stage_x{0.0F};

    /** @brief Vertical position in stage logical coordinates. */
    float stage_y{0.0F};

    /** @brief Pointer button index; 0 is the primary button. */
    int button{0};

    /** @brief Portable key name for key events. */
    InputKey key{InputKey::unknown};

    /** @brief Modifier state captured with the event. */
    InputModifiers modifiers;

    /** @brief Semantic action for navigation events. */
    NavigationAction navigation{NavigationAction::unknown};

    /** @brief Stable source-device identifier for navigation events. */
    std::int32_t input_device_id{0};

    /** @brief Whether a key event is an OS auto-repeat. */
    bool repeat{false};

    /** @brief Mark the event handled without stopping propagation. */
    void handle() noexcept;

    /** @brief Stop propagation; later phases and listeners are skipped. */
    void stop() noexcept { stopped_ = true; }

    /**
     * @brief Handle and stop the event, recording the current actor.
     *
     * Cancellation is used when a pointer contact is abandoned (for example
     * after a gesture is cancelled by the OS) so actors can release capture.
     */
    void cancel() noexcept
    {
        cancelled_ = true;
        handled_ = true;
        stopped_ = true;
        if (!handled_by_) handled_by_ = current_target_;
    }

    /**
     * @brief Read the handled flag.
     * @return true when any listener or handler handled the event.
     */
    [[nodiscard]] bool handled() const noexcept { return handled_; }

    /**
     * @brief Read the stopped flag.
     * @return true when propagation has been stopped or cancelled.
     */
    [[nodiscard]] bool stopped() const noexcept { return stopped_; }

    /**
     * @brief Read the cancelled flag.
     * @return true when the event was cancelled.
     */
    [[nodiscard]] bool cancelled() const noexcept { return cancelled_; }

    /**
     * @brief Read the dispatch target.
     * @return Stage-selected or caller-supplied dispatch target.
     */
    [[nodiscard]] Actor* target() const noexcept { return target_; }

    /**
     * @brief Read the actor currently receiving the event.
     * @return Actor receiving the event during this dispatch step.
     */
    [[nodiscard]] Actor* current_target() const noexcept
    {
        return current_target_;
    }

    /**
     * @brief Read the actor that first handled or cancelled the event.
     * @return Actor that first handled or cancelled the event, or null.
     */
    [[nodiscard]] Actor* handled_by() const noexcept { return handled_by_; }

    /**
     * @brief Read the current dispatch phase.
     * @return Current capture, target, or bubble phase.
     */
    [[nodiscard]] InputPhase phase() const noexcept { return phase_; }

    /**
     * @brief Read the horizontal position relative to the current target.
     * @return Local horizontal position in logical units.
     */
    [[nodiscard]] float local_x() const noexcept { return local_x_; }

    /**
     * @brief Read the vertical position relative to the current target.
     * @return Local vertical position in logical units.
     */
    [[nodiscard]] float local_y() const noexcept { return local_y_; }

private:
    friend class Stage;

    Actor* target_{nullptr};
    Actor* current_target_{nullptr};
    Actor* handled_by_{nullptr};
    InputPhase phase_{InputPhase::target};
    float local_x_{0.0F};
    float local_y_{0.0F};
    bool handled_{false};
    bool stopped_{false};
    bool cancelled_{false};
};

/** @brief Callback invoked for each phase during input dispatch. */
using InputListener = std::function<void(InputEvent&)>;

/** @brief Stable actor-local listener identifier; insertions increase it. */
using InputListenerId = std::uint64_t;

} // namespace squared::scene2d
