module sq.term;

import std;

namespace sq::term::detail
{

namespace
{

std::size_t dirty_words( std::uint16_t rows ) noexcept
{
    return ( std::size_t{ rows } + 63U ) / 64U;
}

} // namespace

void Grid::resize( std::uint16_t columns, std::uint16_t rows, bool keep, const Cell &fill )
{
    columns = std::max<std::uint16_t>( columns, 1 );
    rows = std::max<std::uint16_t>( rows, 1 );
    const std::size_t count = std::size_t{ columns } * rows;
    auto cells = std::make_unique_for_overwrite<Cell[]>( count );
    std::fill_n( cells.get(), count, fill );
    auto wrapped = std::make_unique<std::uint8_t[]>( rows );
    if( keep && cells_ ) {
        const std::uint16_t keep_rows = std::min( rows, rows_ );
        const std::uint16_t keep_columns = std::min( columns, columns_ );
        for( std::uint16_t r = 0; r < keep_rows; ++r ) {
            const std::span<const Cell> old = row( r );
            std::copy_n( old.begin(), keep_columns, &cells[std::size_t{ r } * columns] );
            wrapped[r] = wrapped_[map_[r]];
        }
    }
    auto map = std::make_unique_for_overwrite<std::uint16_t[]>( rows );
    std::iota( map.get(), map.get() + rows, std::uint16_t{ 0 } );
    cells_ = std::move( cells );
    map_ = std::move( map );
    wrapped_ = std::move( wrapped );
    dirty_ = std::make_unique<std::uint64_t[]>( dirty_words( rows ) );
    columns_ = columns;
    rows_ = rows;
    mark_all();
}

void Grid::fill( std::uint16_t row, std::uint16_t first, std::uint16_t last,
                 const Cell &fill ) noexcept
{
    if( first > last || first >= columns_ ) {
        return;
    }
    last = std::min<std::uint16_t>( last, columns_ - 1U );
    const std::span<Cell> cells = this->row( row );
    std::fill( cells.begin() + first, cells.begin() + last + 1, fill );
    mark( row );
}

void Grid::clear( const Cell &fill ) noexcept
{
    std::fill_n( cells_.get(), std::size_t{ columns_ } * rows_, fill );
    std::fill_n( wrapped_.get(), rows_, std::uint8_t{ 0 } );
    mark_all();
}

void Grid::scroll_up( std::uint16_t top, std::uint16_t bottom, std::uint16_t count,
                      const Cell &fill ) noexcept
{
    if( top > bottom || bottom >= rows_ ) {
        return;
    }
    const auto height = static_cast<std::uint16_t>( bottom - top + 1 );
    count = std::min( count, height );
    std::rotate( &map_[top], &map_[top + count], &map_[bottom + 1U] );
    for( std::uint16_t r = 0; r < count; ++r ) {
        const auto at = static_cast<std::uint16_t>( bottom - r );
        const std::span<Cell> cells = row( at );
        std::fill( cells.begin(), cells.end(), fill );
        set_wrapped( at, false );
    }
    for( std::uint16_t r = top; r <= bottom; ++r ) {
        mark( r );
    }
}

void Grid::scroll_down( std::uint16_t top, std::uint16_t bottom, std::uint16_t count,
                        const Cell &fill ) noexcept
{
    if( top > bottom || bottom >= rows_ ) {
        return;
    }
    const auto height = static_cast<std::uint16_t>( bottom - top + 1 );
    count = std::min( count, height );
    std::rotate( &map_[top], &map_[bottom + 1U - count], &map_[bottom + 1U] );
    for( std::uint16_t r = 0; r < count; ++r ) {
        const auto at = static_cast<std::uint16_t>( top + r );
        const std::span<Cell> cells = row( at );
        std::fill( cells.begin(), cells.end(), fill );
        set_wrapped( at, false );
    }
    for( std::uint16_t r = top; r <= bottom; ++r ) {
        mark( r );
    }
}

void Grid::mark_all() noexcept
{
    std::fill_n( dirty_.get(), dirty_words( rows_ ), ~std::uint64_t{ 0 } );
}

void Grid::clear_dirty() noexcept
{
    std::fill_n( dirty_.get(), dirty_words( rows_ ), std::uint64_t{ 0 } );
}

} // namespace sq::term::detail
