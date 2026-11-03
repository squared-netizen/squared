export module sq.term:src.grid;

export import :cell;
import std;

export namespace sq::term::detail
{

/**
 * One screen of cells: rows of `columns` cells, in one block allocated when
 * the size is set, with a table from the row on screen to the row in the
 * block. Scrolling turns the table, two bytes a row moved, never the cells.
 * Each row has a dirty bit (changed since the owner last drew it) and a
 * wrapped bit (its text went on into the next row by autowrap).
 *
 * For 80 x 24: 23,040 bytes of cells, 48 of table, 8 of dirty bits, 24 of
 * wrapped flags. Nothing is allocated but by `resize`.
 */
class Grid
{
    public:
        Grid() = default;

        /**
         * The new size, blank (`fill`), or keeping what fits: the top left
         * of the old one, row by row. `columns` and `rows` are at least 1.
         */
        void resize( std::uint16_t columns, std::uint16_t rows, bool keep, const Cell &fill );

        [[nodiscard]] std::uint16_t columns() const noexcept { return columns_; }
        [[nodiscard]] std::uint16_t rows() const noexcept { return rows_; }

        /** Row `row` on screen; `row` is below `rows()`. Does not mark it dirty. */
        [[nodiscard]] std::span<Cell> row( std::uint16_t row ) noexcept
        {
            return { &cells_[std::size_t{ map_[row] } * columns_], columns_ };
        }
        [[nodiscard]] std::span<const Cell> row( std::uint16_t row ) const noexcept
        {
            return { &cells_[std::size_t{ map_[row] } * columns_], columns_ };
        }

        /** Cells `first` to `last` of a row (both included) set to `fill`, the row marked. */
        void fill( std::uint16_t row, std::uint16_t first, std::uint16_t last,
                   const Cell &fill ) noexcept;

        /** Every row set to `fill`, and marked; none wrapped. */
        void clear( const Cell &fill ) noexcept;

        /**
         * Rows `top` to `bottom` (both included) moved up `count` rows, the
         * rows that leave the top coming in at the bottom set to `fill`.
         * Every row of the region is marked.
         */
        void scroll_up( std::uint16_t top, std::uint16_t bottom, std::uint16_t count,
                        const Cell &fill ) noexcept;

        /** The same, down: blank rows in at the top. */
        void scroll_down( std::uint16_t top, std::uint16_t bottom, std::uint16_t count,
                          const Cell &fill ) noexcept;

        void mark( std::uint16_t row ) noexcept
        {
            dirty_[row / 64U] |= std::uint64_t{ 1 } << ( row % 64U );
        }
        void mark_all() noexcept;
        [[nodiscard]] bool dirty( std::uint16_t row ) const noexcept
        {
            return ( ( dirty_[row / 64U] >> ( row % 64U ) ) & 1U ) != 0;
        }
        void clear_dirty() noexcept;

        [[nodiscard]] bool wrapped( std::uint16_t row ) const noexcept
        {
            return wrapped_[map_[row]] != 0;
        }
        void set_wrapped( std::uint16_t row, bool wrapped ) noexcept
        {
            wrapped_[map_[row]] = wrapped ? 1 : 0;
        }

    private:
        std::unique_ptr<Cell[]> cells_;
        std::unique_ptr<std::uint16_t[]> map_;
        std::unique_ptr<std::uint64_t[]> dirty_;
        std::unique_ptr<std::uint8_t[]> wrapped_;
        std::uint16_t columns_ = 0;
        std::uint16_t rows_ = 0;
};

} // namespace sq::term::detail
