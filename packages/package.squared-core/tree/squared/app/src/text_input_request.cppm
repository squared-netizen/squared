export module sq.app:src.text_input_request;

export import :src.text_input_area;
export import :src.text_input_purpose;

export namespace sq::app
{

/**
 * @brief Request sent when platform text input starts.
 *
 * @param area The current on-screen target rectangle.
 * @param purpose The keyboard layout the platform should present.
 */
struct TextInputRequest
{
        /** @brief Current on-screen target rectangle. */
        TextInputArea area{};
        /** @brief Requested soft-keyboard content mode. */
        TextInputPurpose purpose{ TextInputPurpose::normal };
        /**
         * @brief Whether the soft keyboard comes up now. False for a widget
         * that takes text (a hardware keyboard's keys, a keyboard already
         * up) but shows the soft keyboard only when asked
         * (`Widget::keyboard_on_focus`), such as a terminal.
         */
        bool keyboard{ true };
};

} // namespace sq::app
