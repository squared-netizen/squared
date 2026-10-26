#pragma once

#include <squared/app/event.hpp>
#include <squared/app/safe_area.hpp>
#include <squared/app/src/report.hpp>
#include <squared/app/src/text_input_service.hpp>
#include <squared/gui/layout.hpp>
#include <squared/messaging/telegraph.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace sq::app
{
struct Runtime;
} // namespace sq::app

namespace sq::graphics
{
class Context;
} // namespace sq::graphics

namespace sq::gui
{
class Ui;
} // namespace sq::gui

namespace sq
{

class App;

/**
 * @brief The view: a stack of screens, each a layout, with the skin, the
 * drawing, input, and the UI end of the Telegraph.
 *
 * Only the top screen has widgets: a screen that comes back (`back`) is
 * built again from its layout. A button's tap is sent as it reads: a
 * telegram to the model, or a navigation to this stack. The App owns the
 * Stage and calls the rest; an app calls `push`, `replace` and `back`.
 *
 * Navigation asked for while an event is being delivered (a tap) waits until
 * delivery has finished: building the next screen replaces the widgets, and
 * the tapped button is one of them.
 */
class Stage final
{
    public:
        /** @brief How many screens the stack holds at most. */
        static constexpr std::size_t max_depth = 16;

        explicit Stage( messaging::TelegraphUi &telegraph ) noexcept;
        ~Stage();
        Stage( const Stage & ) = delete;
        Stage &operator=( const Stage & ) = delete;
        Stage( Stage && ) = delete;
        Stage &operator=( Stage && ) = delete;

        /**
         * @brief Show `layout` over the screen there is, which keeps nothing
         * but its layout. False when the stack is full (`max_depth`); the
         * problem is reported. `layout` must outlive its place on the stack:
         * a compiled one (`layouts::settings`) does.
         */
        bool push( const gui::Layout &layout ) noexcept;

        /** @brief Show `layout` instead of the screen there is (or as the first). */
        void replace( const gui::Layout &layout ) noexcept;

        /** @brief Back to the screen beneath. False at the root: nothing to go back to. */
        bool back() noexcept;

        /** @brief How many screens there are, once what waits has happened. */
        [[nodiscard]] std::size_t depth() const noexcept;

        /** @brief The screen shown; nullptr when there is none. */
        [[nodiscard]] const gui::Layout *top() const noexcept;

        /**
         * @brief The app's layout called `name`, from its layout files
         * (sqgen's `layouts::all()`); nullptr when there is none.
         */
        [[nodiscard]] static const gui::Layout *find( std::string_view name ) noexcept;

        /** @brief The UI end of the Telegraph: send, and read what the model said. */
        [[nodiscard]] messaging::TelegraphUi &telegraph() noexcept { return telegraph_; }

        /**
         * @brief The top screen's widgets; nullptr while the skin is not
         * loaded (no window yet, or it failed).
         */
        [[nodiscard]] gui::Ui *ui() noexcept;

        /** @brief The skin could not be loaded: the Stage draws dark red. */
        [[nodiscard]] bool failed() const noexcept { return failed_; }

    private:
        friend class App;

        /** @brief A navigation waiting for an event's delivery to finish. */
        struct Change final
        {
                gui::TapKind kind = gui::TapKind::none;
                const gui::Layout *layout = nullptr;
        };

        /** @brief What the window has: the skin's pieces, the batch, the camera, the widgets. */
        struct Drawing;

        // What the App calls.
        void attach( app::Runtime &runtime );
        void detach() noexcept;
        void clear() noexcept;
        [[nodiscard]] bool event( const app::Event &event );
        void update( std::chrono::nanoseconds delta ) noexcept;
        void render( graphics::Context &graphics ) noexcept;
        void resize( int pixel_width, int pixel_height ) noexcept;
        void set_safe_area( const app::SafeArea &area ) noexcept;
        void surface_created( graphics::Context &graphics ) noexcept;
        void set_report( app::Report report ) noexcept { report_ = report; }
        void set_text_input_service( app::TextInputService *service ) noexcept;
        [[nodiscard]] bool leave_requested() const noexcept { return leave_; }

        // The stack.
        void change( gui::TapKind kind, const gui::Layout *layout ) noexcept;
        void apply( Change change ) noexcept;
        void flush() noexcept;
        void show_top() noexcept;
        void lay_out() noexcept;

        // Taps.
        static void tapped( void *context, std::string_view line ) noexcept;
        void tap( std::string_view line ) noexcept;
        void send( std::string_view line ) noexcept;

        void say( const char *format, ... ) noexcept __attribute__( ( format( printf, 2, 3 ) ) );

        messaging::TelegraphUi &telegraph_;
        app::Report report_{};
        app::TextInputService *text_input_ = nullptr;
        std::unique_ptr<Drawing> drawing_;
        std::array<const gui::Layout *, max_depth> screens_{};
        std::array<Change, 4> pending_{};
        app::SafeArea safe_area_{};
        int pixel_width_ = 0;
        int pixel_height_ = 0;
        float logical_width_ = 0.0F;
        float logical_height_ = 0.0F;
        std::uint8_t depth_ = 0;
        std::uint8_t pending_count_ = 0;
        bool dispatching_ = false;
        bool failed_ = false;
        bool broken_ = false;
        bool leave_ = false;
};

static_assert( sizeof( Stage ) == 264, "Stage: 16 screens and 4 waiting changes, as pointers" );

} // namespace sq
