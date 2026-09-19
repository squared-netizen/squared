#pragma once

#include <squared/gui/window.hpp>

#include <functional>
#include <string>
#include <string_view>

namespace sq::gui {

class Table;

/**
 * @brief Modal Window with separate content and action-button tables.
 */
class Dialog final : public Window {
public:
    /** @brief Action invoked with the chosen result name. */
    using ResultCallback = std::function<void(std::string_view)>;

    /**
     * @brief Construct a dialog.
     * @param title Title-bar text.
     * @param result Result action; invoked when any button is chosen.
     */
    explicit Dialog(std::string title = {}, ResultCallback result = {});

    /**
     * @brief Access the dialog content table.
     * @return Reference to the content table for the message body.
     */
    [[nodiscard]] Table& dialog_content() noexcept { return *dialog_content_; }

    /**
     * @brief Access the action-button table.
     * @return Reference to the table receiving configured buttons.
     */
    [[nodiscard]] Table& button_table() noexcept { return *buttons_; }

    /**
     * @brief Append a message paragraph.
     * @param text Message text to display.
     * @return This dialog for chaining.
     */
    Dialog& text(std::string text);

    /**
     * @brief Append an action button.
     * @param text Button label.
     * @param result Result name reported when the button is chosen.
     * @return This dialog for chaining.
     */
    Dialog& button(std::string text, std::string result);

    /**
     * @brief Set the result action.
     * @param result Action copied into the dialog; invoked with the chosen
     * result name.
     */
    void set_on_result(ResultCallback result);

    /**
     * @brief Control Escape-to-cancel behavior.
     * @param enabled true reports a cancel result when Escape is pressed.
     */
    void set_cancel_on_escape(bool enabled) noexcept { cancel_on_escape_ = enabled; }

protected:
    /**
     * @brief Report Escape closing for dialogs.
     * @return true, unless cancel-on-escape was disabled.
     */
    [[nodiscard]] bool escape_closes() const noexcept override;

private:
    void choose(std::string result);
    Table* dialog_content_{nullptr};
    Table* buttons_{nullptr};
    ResultCallback result_;
    bool cancel_on_escape_{true};
};

} // namespace sq::gui
