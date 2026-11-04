export module sq.term:terminal;

export import :cell;
export import :cursor;
export import :src.grid;
export import :src.parser;
export import :src.scrollback;
export import :terminal_config;
export import :terminal_events;
export import :terminal_modes;
export import :text_point;
import std;

export namespace sq::term
{

/**
 * @brief A terminal: the bytes a program writes in, a screen of cells out.
 *
 * It reads ECMA-48 (control functions, control sequences, SGR colours and
 * attributes, operating system commands) over UTF-8, with the DEC and xterm
 * functions sqterm takes: the alternate screen, scroll regions, cursor keys
 * and visibility, bracketed paste. Compatibility with other terminals comes
 * last (decided 2026-11-02): what it does not implement it ignores.
 *
 * It owns no pty, draws nothing and reads no keys: its owner feeds it what
 * it reads from the program, draws the rows marked dirty, and writes back
 * the replies (a cursor position report) to the program. So it runs on any
 * thread, and in a test with no program at all.
 *
 * Memory: the two screens (rows x columns x 12 bytes each), the scrollback
 * (1 MB unless configured), allocated by the constructor and by `resize`;
 * `feed` never allocates. 80 x 24 with the default scrollback: about 1.05 MB.
 *
 * @code
 * sq::term::Terminal terminal{ { .columns = 80, .rows = 24 } };
 * terminal.feed( "\x1b[1mbold\x1b[0m plain\r\n" );
 * auto first = terminal.row( 0 ); // first[0].code == U'b', bold
 * @endcode
 */
class Terminal final
{
    public:
        explicit Terminal( const TerminalConfig &config = {} );

        Terminal( const Terminal & ) = delete;
        Terminal &operator=( const Terminal & ) = delete;
        Terminal( Terminal && ) noexcept = default;
        Terminal &operator=( Terminal && ) noexcept = default;
        ~Terminal() = default;

        /** What the program wrote, in any pieces: a sequence split between two calls is read whole. */
        void feed( std::span<const std::byte> bytes ) noexcept;
        void feed( std::string_view text ) noexcept { feed( std::as_bytes( std::span{ text } ) ); }

        /**
         * A new size (at least 1 x 1). The main screen keeps its top left;
         * when it loses rows below the cursor, the rows above go to the
         * scrollback first, so the cursor's line stays in view. Lines are
         * not rewrapped. Every row is marked dirty.
         */
        void resize( std::uint16_t columns, std::uint16_t rows );

        [[nodiscard]] std::uint16_t columns() const noexcept { return screen().columns(); }
        [[nodiscard]] std::uint16_t rows() const noexcept { return screen().rows(); }

        /** Row `row` of the screen in use (the alternate one while a program has it). */
        [[nodiscard]] std::span<const Cell> row( std::uint16_t row ) const noexcept
        {
            return screen().row( row );
        }

        /** Changed since `clear_dirty`. */
        [[nodiscard]] bool row_dirty( std::uint16_t row ) const noexcept
        {
            return screen().dirty( row );
        }
        void clear_dirty() noexcept { active().clear_dirty(); }

        [[nodiscard]] Cursor cursor() const noexcept { return cursor_; }
        [[nodiscard]] TerminalModes modes() const noexcept { return modes_; }

        /** Lines scrolled off the top of the main screen. */
        [[nodiscard]] std::size_t history_size() const noexcept { return scrollback_.size(); }
        /** Line `back`, 0 the newest; `back` is below `history_size()`. */
        [[nodiscard]] std::span<const Cell> history_line( std::size_t back ) const noexcept
        {
            return scrollback_.line( back ).cells;
        }

        /**
         * Its lines are numbered from the first it took: the scrollback's
         * oldest kept line is `first_line()`, the screen's row 0 is
         * `screen_line()`, row r is `screen_line() + r`. A line keeps its
         * number as more scroll off (they move up the scrollback, then out).
         */
        [[nodiscard]] std::uint64_t first_line() const noexcept
        {
            return scrollback_.pushed() - scrollback_.size();
        }
        [[nodiscard]] std::uint64_t screen_line() const noexcept { return scrollback_.pushed(); }

        /** Row `row` of the screen in use went on into the next by autowrap. */
        [[nodiscard]] bool row_wrapped( std::uint16_t row ) const noexcept
        {
            return screen().wrapped( row );
        }

        /**
         * The text from `from` to `to` (both included, either first), as
         * UTF-8, into `out`; the bytes it takes, all of them, whatever `out`
         * holds. Scrollback lines, then the screen in use's rows, held to
         * the lines there are. A line that wrapped runs on into the next; any
         * other ends with LF, its trailing blanks left out. A wide
         * character's second cell gives nothing.
         *
         * `out` takes whole characters, in order, until one does not fit:
         * call it with an empty `out` for the size, then with that much.
         * No allocation; finding the first scrollback line walks back to it.
         */
        std::size_t copy_text( TextPoint from, TextPoint to, std::span<char> out ) const noexcept;

        /** The title a program set (OSC 0 or 2), UTF-8, at most 128 bytes. */
        [[nodiscard]] std::string_view title() const noexcept
        {
            return { title_.data(), title_size_ };
        }

        /** What happened since the last call: the bell, a new title. */
        [[nodiscard]] TerminalEvents take_events() noexcept { return std::exchange( events_, {} ); }

        /**
         * Bytes for the program (replies to a device status report or a
         * device attributes request), for the owner to write to it; then
         * `clear_replies`. At most 256 wait; more are dropped.
         */
        [[nodiscard]] std::span<const std::byte> replies() const noexcept
        {
            return std::as_bytes( std::span{ replies_.data(), replies_size_ } );
        }
        void clear_replies() noexcept { replies_size_ = 0; }

    private:
        // What the parser calls (src/terminal.cpp).
        struct Sink;

        struct Saved
        {
                Cursor cursor{};
                Cell pen{};
                bool pending_wrap = false;
        };

        [[nodiscard]] const detail::Grid &screen() const noexcept
        {
            return modes_.alternate_screen ? alternate_ : primary_;
        }
        [[nodiscard]] detail::Grid &active() noexcept
        {
            return modes_.alternate_screen ? alternate_ : primary_;
        }

        void print( char32_t code ) noexcept;
        void execute( std::uint8_t control ) noexcept;
        void escape( const detail::CsiSequence &sequence ) noexcept;
        void csi( const detail::CsiSequence &sequence ) noexcept;
        void osc( std::string_view text ) noexcept;

        void select_graphic_rendition( const detail::CsiSequence &sequence ) noexcept;
        void set_mode( const detail::CsiSequence &sequence, bool on ) noexcept;
        void use_alternate( bool on, bool save_and_clear ) noexcept;
        void line_feed() noexcept;
        void reverse_line_feed() noexcept;
        void scroll_up( std::uint16_t count ) noexcept;
        void scroll_down( std::uint16_t count ) noexcept;
        void move_to( int column, int row ) noexcept;
        void erase_display( std::uint16_t mode ) noexcept;
        void erase_line( std::uint16_t mode ) noexcept;
        void insert_cells( std::uint16_t count ) noexcept;
        void delete_cells( std::uint16_t count ) noexcept;
        void insert_lines( std::uint16_t count ) noexcept;
        void delete_lines( std::uint16_t count ) noexcept;
        void tab_forward( std::uint16_t count ) noexcept;
        void tab_backward( std::uint16_t count ) noexcept;
        void reset_tabs() noexcept;
        [[nodiscard]] bool tab_at( int column ) const noexcept;
        void full_reset() noexcept;
        void reply( std::string_view text ) noexcept;
        void save_cursor() noexcept;
        void restore_cursor() noexcept;
        [[nodiscard]] Cell blank() const noexcept;
        void put( std::uint16_t column, const Cell &cell ) noexcept;

        detail::Grid primary_;
        detail::Grid alternate_;
        detail::Scrollback scrollback_;
        std::unique_ptr<std::uint64_t[]> tabs_;
        detail::Parser parser_;
        Cell pen_{};
        Saved saved_{};
        Saved saved_primary_{};
        std::array<char, 128> title_{};
        std::array<char, 256> replies_{};
        Cursor cursor_{};
        std::uint16_t top_ = 0;
        std::uint16_t bottom_ = 0;
        std::uint16_t title_size_ = 0;
        std::uint16_t replies_size_ = 0;
        char32_t last_printed_ = U' ';
        TerminalModes modes_{};
        TerminalEvents events_{};
        bool pending_wrap_ = false;
};

// 960 bytes on 64-bit Linux and Android (2026-11-02): the parser's 356, the
// title's 128 and the replies' 256 inline. The cells are allocated.
static_assert( sizeof( Terminal ) <= 1024, "a Terminal grew: see docs/developer/term.md" );

} // namespace sq::term
