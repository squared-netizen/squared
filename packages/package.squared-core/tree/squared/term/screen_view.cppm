export module sq.term:screen_view;

export import :src.screen_layout;
import std;

export namespace sq::term
{

/**
 * @brief A screen snapshot as the UI reads it: its size, each row's cells
 * and version, the cursor, the modes, the title, the lines its rows show. A view over the bytes it
 * was made from; nothing is copied.
 *
 * @code
 * if( const auto shot = t.snapshot( screen ) ) {
 *     if( const auto view = sq::term::ScreenView::read( shot->bytes ) ) {
 *         for( std::uint16_t r = 0; r < view->rows(); ++r ) {
 *             if( view->version( r ) != drawn[r] ) { draw_row( r, view->row( r ) ); }
 *         }
 *     }
 * }
 * @endcode
 */
class ScreenView final
{
    public:
        /** The view, when `bytes` are a whole screen snapshot of this layout; else nothing. */
        [[nodiscard]] static std::optional<ScreenView>
        read( std::span<const std::byte> bytes ) noexcept;

        [[nodiscard]] std::uint16_t columns() const noexcept { return header_.columns; }
        [[nodiscard]] std::uint16_t rows() const noexcept { return header_.rows; }
        [[nodiscard]] std::span<const Cell> row( std::uint16_t row ) const noexcept
        {
            return cells_.subspan( std::size_t{ row } * header_.columns, header_.columns );
        }
        /** Moves on each time the row changes. */
        [[nodiscard]] std::uint32_t version( std::uint16_t row ) const noexcept;
        [[nodiscard]] Cursor cursor() const noexcept { return header_.cursor; }
        /** The cursor is to be drawn: the program shows it and the view is not scrolled back. */
        [[nodiscard]] bool cursor_shown() const noexcept
        {
            return header_.modes.cursor_visible && header_.scrolled_back == 0;
        }
        [[nodiscard]] TerminalModes modes() const noexcept { return header_.modes; }
        [[nodiscard]] std::string_view title() const noexcept
        {
            return { header_.title.data(), header_.title_size };
        }
        [[nodiscard]] std::size_t history_size() const noexcept { return header_.history_size; }
        [[nodiscard]] std::size_t scrolled_back() const noexcept { return header_.scrolled_back; }
        /**
         * The terminal's number for the line row 0 shows: row r is line
         * `top_line() + r`, what `Terminal::copy_text` takes.
         */
        [[nodiscard]] std::uint64_t top_line() const noexcept { return header_.top_line; }

    private:
        ScreenView() = default;

        detail::ScreenHeader header_{};
        std::span<const std::byte> versions_;
        std::span<const Cell> cells_;
};

} // namespace sq::term
