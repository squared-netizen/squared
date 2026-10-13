#pragma once

#include <squared/gui/button.hpp>

#include <functional>
#include <string>

namespace sq::gui {

class ButtonGroup;

/** @brief Two-state button that reports changes through a callback. */
class ToggleButton : public Button {
public:
    /** @brief Action invoked with the new checked state. */
    using ChangeCallback = std::function<void(bool)>;

    /**
     * @brief Construct a toggle button.
     * @param text Optional label text.
     * @param checked Initial checked state.
     */
    explicit ToggleButton(std::string text = {}, bool checked = false);
    ~ToggleButton() override;

    /**
     * @brief Set the checked state.
     * @param checked New boolean state; invokes the change callback.
     */
    void set_checked(bool checked);

    /**
     * @brief Read the checked state.
     * @return Current boolean state.
     */
    [[nodiscard]] bool checked() const noexcept { return checked_; }

    /**
     * @brief Set the change callback.
     * @param callback Action copied into the button; invoked with the new
     * state whenever it changes.
     */
    void set_on_change(ChangeCallback callback);

protected:
    /** @brief Toggle the checked state and invoke the change callback. */
    void activate() override;

    /**
     * @brief Report the checked state for selected styling.
     * @return `true` when the toggle button is checked.
     */
    [[nodiscard]] bool selected() const noexcept override;

private:
    friend class ButtonGroup;
    void apply_checked(bool checked);

    bool checked_{false};
    ChangeCallback change_callback_;
    ButtonGroup* group_{nullptr};
};

} // namespace sq::gui
