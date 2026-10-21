#include <squared/messaging/list_window.hpp>

#include <algorithm>
#include <cassert>

namespace sq::messaging
{

ListWindow::ListWindow( std::uint32_t most_rows ) noexcept
    : most_( most_rows )
{
    assert( most_rows >= 1 );
}

void ListWindow::set_count( std::uint32_t count ) noexcept
{
    if( count != count_ ) {
        count_ = count;
        asked_ = false;
    }
}

std::optional<RowSpan> ListWindow::visible( std::uint32_t first, std::uint32_t rows ) noexcept
{
    if( count_ == 0 ) {
        window_ = {};
        asked_ = true; // nothing to ask for until the count changes
        return std::nullopt;
    }
    first = std::min( first, count_ - 1 );
    rows = std::min( { rows, most_, count_ - first } );
    // A screen on each side, as far as the window's size allows.
    const std::uint32_t margin = std::min( rows, ( most_ - rows ) / 2 );

    bool ask = !asked_ || rows != screen_;
    if( !ask ) {
        const std::uint32_t slack = margin / 2;
        const std::uint32_t start = window_.first;
        const std::uint32_t end = window_.first + window_.rows;
        // Near an edge that is not the list's own end.
        const bool near_top = start > 0 && first < start + slack;
        const bool near_bottom = end < count_ && first + rows + slack > end;
        ask = near_top || near_bottom;
    }
    if( !ask ) {
        return std::nullopt;
    }
    const std::uint32_t start = first > margin ? first - margin : 0;
    const std::uint32_t end = std::min( { first + rows + margin, count_, start + most_ } );
    window_ = { start, end - start };
    screen_ = rows;
    asked_ = true;
    return window_;
}

} // namespace sq::messaging
