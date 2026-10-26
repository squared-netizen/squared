#pragma once

#include <squared/app/event.hpp>
#include <squared/app/safe_area.hpp>
#include <squared/app/src/report.hpp>
#include <squared/app/src/text_input_service.hpp>
#include <squared/gui/layout.hpp>
#include <squared/gui/layout_writer.hpp>
#include <squared/messaging/telegraph.hpp>

#include <array>
#include <chrono>
#include <concepts>
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
class Widget;
} // namespace sq::gui

namespace sq
{

class App;

/**
 * @brief A screen written in C++: a type with a name and a build, no object.
 *
 * ```cpp
 * struct About {
 *     static constexpr std::string_view name = "about";
 *     static void build( sq::gui::LayoutWriter &root );
 * };
 * ```
 *
 * `build` writes the screen's table, as a layout file compiles to; it runs
 * each time the screen is shown. The name is what a tap navigates to
 * (`push about`); lowercase words, digits and `_`.
 */
template<typename S>
concept ScreenType = requires( gui::LayoutWriter &root ) {
    { S::name } -> std::convertible_to<std::string_view>;
    S::build( root );
};

/** @brief Makes a custom node's widget (a layout's `"type": "custom"`); nullptr for none. */
using CustomMaker = std::unique_ptr<gui::Widget> ( * )();

namespace detail
{

/** @brief A C++ screen, as the Stage keeps it: its name and its build. */
struct ScreenEntry final
{
        std::string_view name;
        void ( *build )( gui::LayoutWriter &root ) = nullptr;
};

/** @brief The one entry of the screen type `S`. */
template<ScreenType S>
inline constexpr ScreenEntry screen_entry{ std::string_view( S::name ), &S::build };

} // namespace detail

/**
 * @brief The view: a stack of screens, each a layout or a C++ screen, with
 * the skin, the drawing, input, and the UI end of the Telegraph.
 *
 * Only the top screen has widgets: a screen that comes back (`back`) is
 * built again (a layout from its table, a C++ screen by its `build`). A
 * button's tap is sent as it reads: a telegram to the model, or a navigation
 * to this stack. The App owns the Stage and calls the rest; an app calls
 * `push`, `replace`, `back`, and `add` and `custom` to name what its layout
 * files reach.
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

        /** @brief How many C++ screens, and how many custom types, can be added. */
        static constexpr std::size_t max_added = 16;

        /** @brief The scratch a C++ screen writes into: nodes, bytes of text, numbers. */
        static constexpr std::size_t scratch_nodes = 256;
        static constexpr std::size_t scratch_text = 8192;
        static constexpr std::size_t scratch_numbers = 256;

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

        /** @brief Show the C++ screen `S` over the screen there is; as `push( layout )`. */
        template<ScreenType S>
        bool push() noexcept
        {
            return push_screen( detail::screen_entry<S> );
        }

        /** @brief Show the C++ screen `S` instead of the screen there is (or as the first). */
        template<ScreenType S>
        void replace() noexcept
        {
            change( gui::TapKind::replace, { nullptr, &detail::screen_entry<S> } );
        }

        /** @brief Back to the screen beneath. False at the root: nothing to go back to. */
        bool back() noexcept;

        /**
         * @brief Let taps reach the C++ screen `S` by its name (`push about`),
         * as names.json's `screens` lists it. In `squared_main`. False, and
         * reported, when `max_added` are already added or another has the name.
         */
        template<ScreenType S>
        bool add() noexcept
        {
            return add_screen( detail::screen_entry<S> );
        }

        /**
         * @brief Make a layout's custom node `name` with `make`. `name` must
         * outlive the Stage: a literal does. False, and reported, when
         * `max_added` are already there or `name` is.
         */
        bool custom( std::string_view name, CustomMaker make ) noexcept;

        /** @brief How many screens there are, once what waits has happened. */
        [[nodiscard]] std::size_t depth() const noexcept;

        /**
         * @brief The table of the screen shown: a layout, or what a C++
         * screen wrote (valid while it is shown); nullptr when there is
         * none, or it is not built (no window yet).
         */
        [[nodiscard]] const gui::Layout *top() const noexcept;

        /** @brief The name of the screen shown; empty when there is none. */
        [[nodiscard]] std::string_view top_name() const noexcept;

        /** @brief Whether the C++ screen `S` is the one shown. */
        template<ScreenType S>
        [[nodiscard]] bool showing() const noexcept
        {
            return depth_ != 0 && screens_[depth_ - 1].screen == &detail::screen_entry<S>;
        }

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

        /** @brief A screen on the stack: a layout, or a C++ screen. */
        struct Shown final
        {
                const gui::Layout *layout = nullptr;
                const detail::ScreenEntry *screen = nullptr;
        };

        /** @brief A navigation waiting for an event's delivery to finish. */
        struct Change final
        {
                gui::TapKind kind = gui::TapKind::none;
                Shown shown;
        };

        /** @brief A custom node's type, and what makes it. */
        struct Custom final
        {
                std::string_view name;
                CustomMaker make = nullptr;
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
        void check_screens() noexcept;

        // The stack.
        bool push_screen( const detail::ScreenEntry &screen ) noexcept;
        bool add_screen( const detail::ScreenEntry &screen ) noexcept;
        [[nodiscard]] Shown resolve( std::string_view name ) const noexcept;
        [[nodiscard]] const detail::ScreenEntry *added( std::string_view name ) const noexcept;
        void change( gui::TapKind kind, Shown shown ) noexcept;
        void apply( Change change ) noexcept;
        void flush() noexcept;
        void show_top() noexcept;
        void write( const detail::ScreenEntry &screen ) noexcept;
        void lay_out() noexcept;

        // Taps and custom nodes.
        static void tapped( void *context, std::string_view line ) noexcept;
        static std::unique_ptr<gui::Widget> made( void *context, std::string_view name );
        void tap( std::string_view line ) noexcept;
        void send( std::string_view line ) noexcept;

        void say( const char *format, ... ) noexcept __attribute__( ( format( printf, 2, 3 ) ) );

        messaging::TelegraphUi &telegraph_;
        app::Report report_{};
        app::TextInputService *text_input_ = nullptr;
        std::unique_ptr<Drawing> drawing_;
        std::array<Shown, max_depth> screens_{};
        std::array<Change, 4> pending_{};
        std::array<const detail::ScreenEntry *, max_added> added_{};
        std::array<Custom, max_added> customs_{};
        app::SafeArea safe_area_{};
        int pixel_width_ = 0;
        int pixel_height_ = 0;
        float logical_width_ = 0.0F;
        float logical_height_ = 0.0F;
        std::uint8_t depth_ = 0;
        std::uint8_t pending_count_ = 0;
        std::uint8_t added_count_ = 0;
        std::uint8_t custom_count_ = 0;
        bool dispatching_ = false;
        bool failed_ = false;
        bool broken_ = false;
        bool leave_ = false;
};

// 16 screens and 4 waiting changes, as pointers; 16 added screens and 16
// custom types, as a pointer and a name each.
static_assert( sizeof( Stage ) == 936 );

} // namespace sq
