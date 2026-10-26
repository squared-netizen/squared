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

namespace sq::files
{
class FileSystem;
} // namespace sq::files

namespace sq::graphics
{
class Context;
} // namespace sq::graphics

namespace sq::gui
{
class Ui;
class Widget;
class Window;
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
 *
 * The same type opens as a pane (`open<S>()`, `open about`). Two constants,
 * both optional, say how it shows then, as a pane file's `place` and `modal`:
 *
 * ```cpp
 *     static constexpr sq::gui::PanePlace place = sq::gui::PanePlace::bottom;
 *     static constexpr bool modal = true;
 * ```
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

/** @brief A C++ screen, as the Stage keeps it: its name, its build, how it shows as a pane. */
struct ScreenEntry final
{
        std::string_view name;
        void ( *build )( gui::LayoutWriter &root ) = nullptr;
        gui::PaneShape pane{};
};

/** @brief How the screen type `S` shows as a pane: its `place` and `modal`, if it has them. */
template<typename S>
[[nodiscard]] consteval gui::PaneShape shape_of() noexcept
{
    gui::PaneShape shape;
    if constexpr( requires { S::place; } ) {
        shape.place = S::place;
    }
    if constexpr( requires { S::modal; } ) {
        shape.modal = S::modal;
    }
    return shape;
}

/** @brief A test's way in: defined by a test, never by the framework. */
struct StageProbe;

/** @brief The one entry of the screen type `S`. */
template<ScreenType S>
inline constexpr ScreenEntry screen_entry{ std::string_view( S::name ), &S::build, shape_of<S>() };

} // namespace detail

/**
 * @brief The view: a stack of screens, each a layout or a C++ screen, with
 * the skin, the drawing, input, and the UI end of the Telegraph.
 *
 * Only the top screen has widgets: a screen that comes back (`back`) is
 * built again (a layout from its table, a C++ screen by its `build`). Panes
 * open over the top screen and belong to it: leaving the screen closes them.
 * A button's tap is sent as it reads: a telegram to the model, or a
 * navigation to this stack. The App owns the Stage and calls the rest; an app
 * calls `push`, `replace`, `back`, `open`, `close`, and `add` and `custom` to
 * name what its layout files reach.
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

        /** @brief How many panes can be open over a screen at once. */
        static constexpr std::size_t max_panes = 4;

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
         * but its layout, and closes its panes. False when the stack is full
         * (`max_depth`) or `layout` is a pane; the problem is reported.
         * `layout` must outlive its place on the stack: a compiled one
         * (`layouts::settings`) does.
         */
        bool push( const gui::Layout &layout ) noexcept;

        /**
         * @brief Show `layout` instead of the screen there is (or as the
         * first). False, and reported, when `layout` is a pane.
         */
        bool replace( const gui::Layout &layout ) noexcept;

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
            change( { gui::TapKind::replace, top_pane, { nullptr, &detail::screen_entry<S> } } );
        }

        /**
         * @brief What the back button does: close the top pane if one is
         * open, or else go back to the screen beneath. False at the root
         * with no pane open: nothing to go back to.
         */
        bool back() noexcept;

        /**
         * @brief Open the pane `layout` over the screen shown: its file's
         * `place` and `modal` say where and how. False, and reported, when
         * `max_panes` are open or `layout` is a screen. The pane is built
         * when it opens and freed when it closes; it outlives nothing.
         */
        bool open( const gui::Layout &layout ) noexcept;

        /** @brief Open the C++ screen `S` as a pane; its `place` and `modal`, if it has them. */
        template<ScreenType S>
        bool open() noexcept
        {
            return open_screen( detail::screen_entry<S> );
        }

        /** @brief Close the top pane. False when none is open. */
        bool close() noexcept;

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

        /** @brief How many panes are open, once what waits has happened. */
        [[nodiscard]] std::size_t panes() const noexcept;

        /**
         * @brief The table of the screen shown: a layout, or what a C++
         * screen wrote (valid while it is shown, until a C++ pane is
         * written over it: they share one scratch); nullptr when there is
         * none, or it is not built (no window yet).
         */
        [[nodiscard]] const gui::Layout *top() const noexcept;

        /** @brief The name of the screen shown; empty when there is none. */
        [[nodiscard]] std::string_view top_name() const noexcept;

        /** @brief The name of the top pane; empty when none is open. */
        [[nodiscard]] std::string_view pane_name() const noexcept;

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
         * @brief The top screen's widgets, its panes' among them; nullptr
         * while the skin is not loaded (no window yet, or it failed).
         */
        [[nodiscard]] gui::Ui *ui() noexcept;

        /** @brief The skin could not be loaded: the Stage draws dark red. */
        [[nodiscard]] bool failed() const noexcept { return failed_; }

    private:
        friend class App;
        friend struct detail::StageProbe;

        /** @brief A screen on the stack: a layout, or a C++ screen. */
        struct Shown final
        {
                const gui::Layout *layout = nullptr;
                const detail::ScreenEntry *screen = nullptr;
        };

        /** @brief `Change::pane` for the top pane, whichever it is. */
        static constexpr std::uint8_t top_pane = 0xFF;

        /** @brief A navigation waiting for an event's delivery to finish. */
        struct Change final
        {
                gui::TapKind kind = gui::TapKind::none;
                /// The pane a `close` closes: its slot, or `top_pane`.
                std::uint8_t pane = top_pane;
                Shown shown;
        };

        /**
         * @brief A slot a pane opens in: what it shows, and its window while
         * built. A slot keeps its address while the pane is open, so its
         * buttons' taps say which pane they are in (`close`).
         */
        struct Pane final
        {
                Stage *stage = nullptr;
                Shown shown;
                gui::Window *window = nullptr;
                /// 0: free; else its place among the open panes, 1 the lowest.
                std::uint8_t order = 0;
                gui::PanePlace place = gui::PanePlace::center;
        };

        /** @brief The screens and panes there will be, once what waits has happened. */
        struct Counts final
        {
                std::size_t depth = 0;
                std::size_t panes = 0;
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
        bool open_screen( const detail::ScreenEntry &screen ) noexcept;
        bool add_screen( const detail::ScreenEntry &screen ) noexcept;
        [[nodiscard]] Shown resolve( std::string_view name ) const noexcept;
        [[nodiscard]] const detail::ScreenEntry *added( std::string_view name ) const noexcept;
        [[nodiscard]] Counts after() const noexcept;
        void change( Change change ) noexcept;
        void apply( Change change ) noexcept;
        void flush() noexcept;
        void show_top() noexcept;
        void write( const detail::ScreenEntry &screen, gui::LayoutKind kind ) noexcept;
        [[nodiscard]] bool built( const gui::Layout &layout, gui::LayoutCheck check ) noexcept;
        void lay_out() noexcept;

        // Panes.
        void open_pane( Shown shown ) noexcept;
        void build_pane( Pane &pane ) noexcept;
        void close_pane( Pane &pane ) noexcept;
        void drop_panes() noexcept;
        void place_panes() noexcept;
        [[nodiscard]] Pane *top_pane_slot() noexcept;
        [[nodiscard]] const Pane *top_pane_slot() const noexcept;

        // Taps and custom nodes.
        static void tapped( void *context, std::string_view line ) noexcept;
        static void tapped_in_pane( void *context, std::string_view line ) noexcept;
        static std::unique_ptr<gui::Widget> made( void *context, std::string_view name );
        static std::unique_ptr<gui::Widget> made_in_pane( void *context, std::string_view name );
        void tap( std::string_view line ) noexcept;
        void send( std::string_view line ) noexcept;

        void say( const char *format, ... ) noexcept __attribute__( ( format( printf, 2, 3 ) ) );

        messaging::TelegraphUi &telegraph_;
        app::Report report_{};
        app::TextInputService *text_input_ = nullptr;
        /// Where the skin is read from; nullptr: the compiled-in default. A test's.
        files::FileSystem *skin_files_ = nullptr;
        std::unique_ptr<Drawing> drawing_;
        std::array<Shown, max_depth> screens_{};
        std::array<Change, 4> pending_{};
        std::array<const detail::ScreenEntry *, max_added> added_{};
        std::array<Custom, max_added> customs_{};
        std::array<Pane, max_panes> panes_{};
        app::SafeArea safe_area_{};
        int pixel_width_ = 0;
        int pixel_height_ = 0;
        float logical_width_ = 0.0F;
        float logical_height_ = 0.0F;
        std::uint8_t depth_ = 0;
        std::uint8_t pending_count_ = 0;
        std::uint8_t added_count_ = 0;
        std::uint8_t custom_count_ = 0;
        std::uint8_t pane_count_ = 0;
        bool dispatching_ = false;
        bool failed_ = false;
        bool broken_ = false;
        bool leave_ = false;
};

// 16 screens and 4 waiting changes, as pointers; 16 added screens and 16
// custom types, as a pointer and a name each; 4 pane slots of 40 bytes.
static_assert( sizeof( Stage ) == 1112 );

} // namespace sq
