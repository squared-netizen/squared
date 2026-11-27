export module sq.tui:renderer;

export import :screen;
import sq.term;
import std;

export namespace sq::tui
{

/**
 * @brief Turns frames into the bytes that show them: only what changed
 * since the last frame.
 *
 * It keeps the frame it last wrote and, for the next, writes the cells
 * that differ: runs of them, the cursor moved only where a run starts, the
 * attributes set only where they change. A frame is wrapped in
 * synchronized output (mode 2026), so a terminal that knows it never shows
 * half of one; one that does not ignores it. Colours are 24-bit; text is
 * UTF-8. The bytes are VT/xterm's, which every terminal that matters
 * speaks: no terminfo.
 */
class Renderer
{
    public:
        /**
         * The bytes for `screen`: empty when it shows what the last frame
         * did. Valid until the next call.
         */
        [[nodiscard]] std::string_view render( const Screen &screen );

        /** The next frame is written whole (after the terminal was cleared, or resized). */
        void invalidate() noexcept { whole_ = true; }

    private:
        void move_to( std::uint16_t x, std::uint16_t y );
        void set_style( const sq::term::Cell &cell );
        void put_code( char32_t code );

        std::vector<sq::term::Cell> last_;
        std::uint16_t columns_ = 0;
        std::uint16_t rows_ = 0;
        std::optional<Point> last_cursor_;
        bool whole_ = true;

        // While a frame is written: where the terminal's cursor is, and its style.
        std::string out_;
        int x_ = -1;
        int y_ = -1;
        std::optional<Style> style_;
};

} // namespace sq::tui
