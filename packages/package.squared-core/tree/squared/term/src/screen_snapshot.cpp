module sq.term;

import std;

namespace sq::term
{

std::size_t ScreenPublisher::write( Terminal &terminal, std::span<std::byte> out )
{
    const std::uint16_t columns = terminal.columns();
    const std::uint16_t rows = terminal.rows();
    const std::size_t bytes = detail::screen_bytes( columns, rows );
    if( out.size() < bytes ) {
        return 0;
    }
    const std::size_t history = terminal.history_size();
    const std::size_t back = std::min( wanted_back_, history );
    // Every row is new when the size or the view moved, or, scrolled back,
    // when the history grew (the rows shown slid up).
    const bool everything = columns != columns_ || rows != rows_ || back != shown_back_ ||
                            ( back > 0 && history != shown_history_ );
    if( versions_.size() != rows ) {
        versions_.resize( rows, 0 );
    }
    for( std::uint16_t r = 0; r < rows; ++r ) {
        if( everything || terminal.row_dirty( r ) ) {
            ++versions_[r];
        }
    }
    terminal.clear_dirty();
    columns_ = columns;
    rows_ = rows;
    shown_back_ = back;
    shown_history_ = history;

    detail::ScreenHeader header;
    header.columns = columns;
    header.rows = rows;
    header.cursor = terminal.cursor();
    header.modes = terminal.modes();
    const std::string_view title = terminal.title();
    header.title_size = static_cast<std::uint8_t>( std::min( title.size(), header.title.size() ) );
    std::copy_n( title.begin(), header.title_size, header.title.begin() );
    header.history_size = static_cast<std::uint32_t>( history );
    header.scrolled_back = static_cast<std::uint32_t>( back );
    std::memcpy( out.data(), &header, sizeof header );
    std::memcpy( out.data() + sizeof header, versions_.data(),
                 std::size_t{ rows } * sizeof( std::uint32_t ) );

    std::byte *cells = out.data() + detail::cells_offset( rows );
    const std::size_t row_bytes = std::size_t{ columns } * sizeof( Cell );
    for( std::uint16_t r = 0; r < rows; ++r ) {
        std::byte *to = cells + r * row_bytes;
        // Row r of the view: back into the history by `back` lines.
        const auto at = static_cast<std::ptrdiff_t>( r ) - static_cast<std::ptrdiff_t>( back );
        std::span<const Cell> line =
            at >= 0 ? terminal.row( static_cast<std::uint16_t>( at ) )
                    : terminal.history_line( static_cast<std::size_t>( -at - 1 ) );
        const std::size_t kept = std::min<std::size_t>( line.size(), columns );
        std::memcpy( to, line.data(), kept * sizeof( Cell ) );
        // A history line is trimmed, or a narrower screen's: blanks after it.
        for( std::size_t c = kept; c < columns; ++c ) {
            const Cell blank{};
            std::memcpy( to + c * sizeof( Cell ), &blank, sizeof blank );
        }
    }
    return bytes;
}

std::optional<ScreenView> ScreenView::read( std::span<const std::byte> bytes ) noexcept
{
    if( bytes.size() < sizeof( detail::ScreenHeader ) ) {
        return std::nullopt;
    }
    ScreenView view;
    std::memcpy( &view.header_, bytes.data(), sizeof view.header_ );
    const detail::ScreenHeader &header = view.header_;
    if( header.layout != detail::ScreenHeader::current ||
        bytes.size() < detail::screen_bytes( header.columns, header.rows ) ||
        header.title_size > header.title.size() ) {
        return std::nullopt;
    }
    view.versions_ =
        bytes.subspan( sizeof header, std::size_t{ header.rows } * sizeof( std::uint32_t ) );
    // The cells were written as `Cell`s into storage made for them (the
    // channel's byte buffers, aligned for any type), at a multiple of 4.
    const std::byte *cells = bytes.data() + detail::cells_offset( header.rows );
    view.cells_ = { reinterpret_cast<const Cell *>( cells ),
                    std::size_t{ header.columns } * header.rows };
    return view;
}

std::uint32_t ScreenView::version( std::uint16_t row ) const noexcept
{
    std::uint32_t version = 0;
    std::memcpy( &version, versions_.data() + std::size_t{ row } * sizeof version, sizeof version );
    return version;
}

} // namespace sq::term
