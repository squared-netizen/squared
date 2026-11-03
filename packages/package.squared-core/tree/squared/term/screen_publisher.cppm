export module sq.term:screen_publisher;

export import :src.screen_layout;
export import :terminal;
import std;

export namespace sq::term
{

/**
 * @brief A terminal's screen written as a snapshot, on the model's thread,
 * for a `ScreenView` on the UI's (the app model's third channel: the model
 * publishes it through the Telegraph, `Telegraph::snapshot` and `publish`).
 *
 * Each publish is the whole screen (12 bytes a cell: 24 KB for 50 x 40),
 * because the UI may skip publishes: what it reads must stand alone. What
 * lets the UI redraw only what changed is a version for each row, moved on
 * when the row changed since the publish before (or every row, when the
 * size or the view moved): the UI redraws the rows whose version is not
 * the one it drew.
 *
 * The view can be scrolled back into the scrollback (`scroll_back`): the
 * rows shown are then the history's, then the screen's, and the cursor is
 * hidden.
 *
 * @code
 * sq::term::ScreenPublisher publisher;
 * auto room = t.snapshot( screen, publisher.bytes_for( terminal ) );
 * t.publish( screen, publisher.write( terminal, room ) );
 * @endcode
 */
class ScreenPublisher final
{
    public:
        /** The bytes `write` needs for `terminal` now. */
        [[nodiscard]] std::size_t bytes_for( const Terminal &terminal ) const noexcept
        {
            return detail::screen_bytes( terminal.columns(), terminal.rows() );
        }

        /**
         * The screen into `out` (at least `bytes_for( terminal )`), the rows
         * marked dirty given new versions, and the terminal's dirty marks
         * cleared. The bytes written; 0 when `out` is too small.
         */
        std::size_t write( Terminal &terminal, std::span<std::byte> out );

        /**
         * The view `lines` back into the scrollback (0: the screen itself,
         * as the default); held to what the scrollback has at each write.
         */
        void scroll_back( std::size_t lines ) noexcept { wanted_back_ = lines; }
        [[nodiscard]] std::size_t scrolled_back() const noexcept { return shown_back_; }

    private:
        // A version for each row: grown with the screen (on the model's thread).
        std::vector<std::uint32_t> versions_;
        std::size_t wanted_back_ = 0;
        std::size_t shown_back_ = 0;
        std::size_t shown_history_ = 0;
        std::uint16_t columns_ = 0;
        std::uint16_t rows_ = 0;
};

} // namespace sq::term
