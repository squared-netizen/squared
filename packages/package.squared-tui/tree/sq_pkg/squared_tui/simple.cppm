export module sq.tui:simple;

export import :key;
export import :screen;
export import :terminal;
import std;

export namespace sq::tui
{

/**
 * @brief The terminal for a first program: draw, show, wait for a key. No
 * events to sort, no resize to handle.
 *
 *     sq::tui::Simple screen;            // raw mode, the alternate screen
 *     int x = 10, y = 5;
 *     for( ;; ) {
 *         screen.clear();
 *         screen.put( x, y, "@" );
 *         screen.show();
 *         const sq::tui::Key key = screen.key();
 *         if( key == 'q' ) break;
 *         if( key == 'l' ) ++x;
 *     }                                  // the terminal as it was
 *
 * A Screen and a Terminal, and nothing of its own: `screen()` and
 * `terminal()` are the real types, for when a program outgrows it. A resize
 * makes the screen the new size and draws it again; a mouse report, a
 * paste or a focus change is not a key and is passed over.
 *
 * When the terminal cannot be opened (the input is not a terminal), it
 * says why on stderr and the program exits with status 1: for a first
 * program, a message rather than a result to check.
 */
class Simple
{
    public:
        explicit Simple( const TerminalOptions &options = {} );

        [[nodiscard]] std::uint16_t width() const noexcept { return screen_.columns(); }
        [[nodiscard]] std::uint16_t height() const noexcept { return screen_.rows(); }

        void clear( const Style &style = {} ) noexcept { screen_.clear( style ); }
        void put( int x, int y, std::string_view text, const Style &style = {} ) noexcept;
        void put( int x, int y, char32_t code, const Style &style = {} ) noexcept;
        /** What was drawn, on the terminal. */
        void show();

        /** Waits for a key. */
        [[nodiscard]] Key key();
        /** A key if one comes within `wait`: for a program that moves on its own. */
        [[nodiscard]] std::optional<Key> key( std::chrono::milliseconds wait );

        [[nodiscard]] Screen &screen() noexcept { return screen_; }
        [[nodiscard]] Terminal &terminal() noexcept { return terminal_; }

    private:
        Terminal terminal_;
        Screen screen_;
};

} // namespace sq::tui
