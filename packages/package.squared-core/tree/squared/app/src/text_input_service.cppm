export module sq.app:src.text_input_service;

export import :src.text_input_area;
export import :src.text_input_request;

export namespace sq::app
{

/**
 * @brief Portable application-to-platform soft-keyboard boundary.
 *
 * This interface is implemented by the platform adapter. Application and GUI
 * code obtains a pointer through Application::set_text_input_service and
 * never depends on SDL or Android keyboard APIs. All methods are called by
 * the widget framework on the application thread; the implementation owns
 * platform IPC.
 */
class TextInputService
{
    public:
        virtual ~TextInputService() = default;

        /**
     * @brief Show the soft keyboard for one focused control.
     * @param request The target rectangle and requested keyboard purpose.
     */
        virtual void start( const TextInputRequest &request ) = 0;

        /**
     * @brief Reposition the soft keyboard when the control moves or resizes.
     * @param area The updated on-screen target rectangle.
     */
        virtual void update_area( const TextInputArea &area ) = 0;

        /** @brief Hide the soft keyboard. Idempotent when already hidden. */
        virtual void stop() = 0;

        /**
     * @brief Report whether platform text input is active.
     * @return true while the soft keyboard is shown for a control.
     */
        [[nodiscard]] virtual bool active() const noexcept = 0;

        /**
         * @brief Puts the soft keyboard up, or away, while text input stays
         * active (keys still reach the widget). Only for a platform with a
         * soft keyboard; elsewhere nothing happens.
         */
        virtual void show_keyboard( bool shown ) { static_cast<void>( shown ); }

        /** @brief Whether the soft keyboard is up, as far as the platform knows. */
        [[nodiscard]] virtual bool keyboard_shown() const noexcept { return active(); }
};

} // namespace sq::app
