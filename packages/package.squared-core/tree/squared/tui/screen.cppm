export module sq.tui:screen;

export import :rect;
export import :style;
import sq.term;
import std;

export namespace sq::tui
{

class View;

/**
 * @brief What a terminal program draws into: a grid of cells, the same
 * `sq::term::Cell` sqterm's emulator holds.
 *
 * Text is UTF-8, placed by the width table the emulator uses
 * (`sq::term::detail::char_width`), so the two cannot disagree: a wide
 * character takes two cells, the second marked as its tail; a character
 * that takes no column (a combining mark) is not drawn (not yet: one code
 * per cell). Everything is clipped to the screen. Nothing reaches a
 * terminal until a `Renderer` (or `Terminal::show`) writes it.
 */
class Screen
{
    public:
        Screen( std::uint16_t columns, std::uint16_t rows );

        [[nodiscard]] std::uint16_t columns() const noexcept { return columns_; }
        [[nodiscard]] std::uint16_t rows() const noexcept { return rows_; }
        [[nodiscard]] Rect area() const noexcept { return { 0, 0, columns_, rows_ }; }

        /** A new size: what fits stays where it was, the rest is blank. */
        void resize( std::uint16_t columns, std::uint16_t rows );

        /** Every cell a blank in `style`. */
        void clear( const Style &style = {} ) noexcept;

        /** The cell at (x, y); a blank outside the screen. */
        [[nodiscard]] const sq::term::Cell &cell( std::uint16_t x, std::uint16_t y ) const noexcept;
        [[nodiscard]] std::span<const sq::term::Cell> row( std::uint16_t y ) const noexcept;

        /** Writes text from (x, y) on one line; returns the column after it. */
        std::uint16_t put( std::uint16_t x, std::uint16_t y, std::string_view text,
                           const Style &style = {} ) noexcept
        {
            return put_within( area(), x, y, text, style );
        }
        std::uint16_t put( std::uint16_t x, std::uint16_t y, char32_t code,
                           const Style &style = {} ) noexcept
        {
            return put_within( area(), x, y, code, style );
        }
        /** Every cell of `rect` the character `code` in `style`. */
        void fill( Rect rect, char32_t code, const Style &style = {} ) noexcept;
        /** A single-line box around `rect`'s edge. */
        void box( Rect rect, const Style &style = {} ) noexcept;

        /** Where the terminal's cursor is shown after the frame; hidden when empty. */
        [[nodiscard]] std::optional<Point> cursor() const noexcept { return cursor_; }
        void show_cursor( Point at ) noexcept { cursor_ = at; }
        void hide_cursor() noexcept { cursor_.reset(); }

        /** A part of the screen, drawn into from its own (0, 0) and clipped to it. */
        [[nodiscard]] View view( Rect rect ) noexcept;

        // What View draws through: the same, clipped to `clip`.
        std::uint16_t put_within( Rect clip, std::uint16_t x, std::uint16_t y,
                                  std::string_view text, const Style &style ) noexcept;
        std::uint16_t put_within( Rect clip, std::uint16_t x, std::uint16_t y, char32_t code,
                                  const Style &style ) noexcept;

    private:
        [[nodiscard]] sq::term::Cell &at( std::uint16_t x, std::uint16_t y ) noexcept
        {
            return cells_[std::size_t{ y } * columns_ + x];
        }
        /** Before a cell is written: a wide character it cuts in two loses its other half. */
        void unsplit( std::uint16_t x, std::uint16_t y ) noexcept;

        std::vector<sq::term::Cell> cells_;
        std::uint16_t columns_ = 0;
        std::uint16_t rows_ = 0;
        std::optional<Point> cursor_;
};

/**
 * @brief A rectangle of a Screen, drawn into from its own top left: what a
 * curses window is, with no state of its own (a Screen and a Rect).
 */
class View
{
    public:
        View( Screen &screen, Rect area ) noexcept;

        [[nodiscard]] Rect area() const noexcept { return area_; }
        [[nodiscard]] std::uint16_t width() const noexcept { return area_.width; }
        [[nodiscard]] std::uint16_t height() const noexcept { return area_.height; }

        std::uint16_t put( std::uint16_t x, std::uint16_t y, std::string_view text,
                           const Style &style = {} ) noexcept;
        std::uint16_t put( std::uint16_t x, std::uint16_t y, char32_t code,
                           const Style &style = {} ) noexcept;
        void fill( Rect rect, char32_t code, const Style &style = {} ) noexcept;
        void clear( const Style &style = {} ) noexcept
        {
            fill( { 0, 0, width(), height() }, U' ', style );
        }
        void box( const Style &style = {} ) noexcept;
        /** A part of this view, from its own top left. */
        [[nodiscard]] View view( Rect rect ) noexcept;

    private:
        [[nodiscard]] Rect absolute( Rect rect ) const noexcept;

        Screen *screen_;
        Rect area_;
};

} // namespace sq::tui
