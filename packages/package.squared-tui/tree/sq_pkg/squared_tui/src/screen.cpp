module sq.tui;

import sq.term;
import std;

namespace sq::tui
{

namespace
{

/** `rect` cut to `within`. */
Rect intersect( Rect rect, Rect within ) noexcept
{
    const std::uint32_t left = std::max( rect.x, within.x );
    const std::uint32_t top = std::max( rect.y, within.y );
    const std::uint32_t right = std::min( rect.right(), within.right() );
    const std::uint32_t bottom = std::min( rect.bottom(), within.bottom() );
    if( right <= left || bottom <= top ) {
        return { static_cast<std::uint16_t>( left ), static_cast<std::uint16_t>( top ), 0, 0 };
    }
    return { static_cast<std::uint16_t>( left ), static_cast<std::uint16_t>( top ),
             static_cast<std::uint16_t>( right - left ),
             static_cast<std::uint16_t>( bottom - top ) };
}

const sq::term::Cell blank{};

} // namespace

Screen::Screen( std::uint16_t columns, std::uint16_t rows )
    : cells_( std::size_t{ columns } * rows )
    , columns_( columns )
    , rows_( rows )
{}

void Screen::resize( std::uint16_t columns, std::uint16_t rows )
{
    std::vector<sq::term::Cell> cells( std::size_t{ columns } * rows );
    const std::uint16_t keep_columns = std::min( columns, columns_ );
    const std::uint16_t keep_rows = std::min( rows, rows_ );
    for( std::uint16_t y = 0; y < keep_rows; ++y ) {
        for( std::uint16_t x = 0; x < keep_columns; ++x ) {
            cells[std::size_t{ y } * columns + x] = at( x, y );
        }
        // A wide character cut by the new right edge goes.
        if( keep_columns > 0 && cells[std::size_t{ y } * columns + keep_columns - 1].flags.wide ) {
            cells[std::size_t{ y } * columns + keep_columns - 1] = {};
        }
    }
    cells_ = std::move( cells );
    columns_ = columns;
    rows_ = rows;
    if( cursor_ && ( cursor_->x >= columns || cursor_->y >= rows ) ) {
        cursor_.reset();
    }
}

void Screen::clear( const Style &style ) noexcept
{
    std::ranges::fill( cells_, cell_of( U' ', style ) );
}

const sq::term::Cell &Screen::cell( std::uint16_t x, std::uint16_t y ) const noexcept
{
    if( x >= columns_ || y >= rows_ ) {
        return blank;
    }
    return cells_[std::size_t{ y } * columns_ + x];
}

std::span<const sq::term::Cell> Screen::row( std::uint16_t y ) const noexcept
{
    if( y >= rows_ ) {
        return {};
    }
    return std::span{ cells_ }.subspan( std::size_t{ y } * columns_, columns_ );
}

void Screen::unsplit( std::uint16_t x, std::uint16_t y ) noexcept
{
    sq::term::Cell &target = at( x, y );
    if( target.flags.wide_tail && x > 0 ) {
        sq::term::Cell &head = at( static_cast<std::uint16_t>( x - 1 ), y );
        head.code = U' ';
        head.flags.wide = false;
    } else if( target.flags.wide && x + 1 < columns_ ) {
        sq::term::Cell &tail = at( static_cast<std::uint16_t>( x + 1 ), y );
        tail.code = U' ';
        tail.flags.wide_tail = false;
    }
}

std::uint16_t Screen::put_within( Rect clip, std::uint16_t x, std::uint16_t y, char32_t code,
                                  const Style &style ) noexcept
{
    clip = intersect( clip, area() );
    if( y < clip.y || y >= clip.bottom() || x < clip.x || x >= clip.right() ) {
        return x;
    }
    const int width = sq::term::detail::char_width( code );
    if( width <= 0 ) {
        return x; // a combining mark, or a control: one code per cell, for now
    }
    unsplit( x, y );
    if( width == 2 ) {
        if( x + 1U >= clip.right() ) {
            at( x, y ) = cell_of( U' ', style ); // no room for both halves
            return static_cast<std::uint16_t>( x + 1 );
        }
        unsplit( static_cast<std::uint16_t>( x + 1 ), y );
        sq::term::Cell head = cell_of( code, style );
        head.flags.wide = true;
        sq::term::Cell tail = cell_of( 0, style );
        tail.flags.wide_tail = true;
        at( x, y ) = head;
        at( static_cast<std::uint16_t>( x + 1 ), y ) = tail;
        return static_cast<std::uint16_t>( x + 2 );
    }
    at( x, y ) = cell_of( code, style );
    return static_cast<std::uint16_t>( x + 1 );
}

std::uint16_t Screen::put_within( Rect clip, std::uint16_t x, std::uint16_t y,
                                  std::string_view text, const Style &style ) noexcept
{
    sq::term::detail::Utf8Decoder decoder;
    using Step = sq::term::detail::Utf8Decoder::Step;
    for( const char c : text ) {
        auto result = decoder.push( static_cast<std::uint8_t>( c ) );
        if( result.step == Step::code_then_again ) {
            x = put_within( clip, x, y, result.code, style );
            result = decoder.push( static_cast<std::uint8_t>( c ) );
        }
        if( result.step == Step::code ) {
            x = put_within( clip, x, y, result.code, style );
        }
    }
    if( decoder.pending() ) {
        x = put_within( clip, x, y, sq::term::detail::Utf8Decoder::replacement, style );
    }
    return x;
}

void Screen::fill( Rect rect, char32_t code, const Style &style ) noexcept
{
    rect = intersect( rect, area() );
    for( std::uint32_t y = rect.y; y < rect.bottom(); ++y ) {
        for( std::uint32_t x = rect.x; x < rect.right(); ) {
            const std::uint16_t next = put_within( rect, static_cast<std::uint16_t>( x ),
                                                   static_cast<std::uint16_t>( y ), code, style );
            x = next > x ? next : x + 1;
        }
    }
}

void Screen::box( Rect rect, const Style &style ) noexcept
{
    if( rect.width < 2 || rect.height < 2 ) {
        return;
    }
    const auto right = static_cast<std::uint16_t>( rect.right() - 1 );
    const auto bottom = static_cast<std::uint16_t>( rect.bottom() - 1 );
    for( std::uint16_t x = static_cast<std::uint16_t>( rect.x + 1 ); x < right; ++x ) {
        put( x, rect.y, U'─', style );
        put( x, bottom, U'─', style );
    }
    for( std::uint16_t y = static_cast<std::uint16_t>( rect.y + 1 ); y < bottom; ++y ) {
        put( rect.x, y, U'│', style );
        put( right, y, U'│', style );
    }
    put( rect.x, rect.y, U'┌', style );
    put( right, rect.y, U'┐', style );
    put( rect.x, bottom, U'└', style );
    put( right, bottom, U'┘', style );
}

View Screen::view( Rect rect ) noexcept
{
    return View{ *this, rect };
}

View::View( Screen &screen, Rect area ) noexcept
    : screen_( &screen )
    , area_( intersect( area, screen.area() ) )
{}

Rect View::absolute( Rect rect ) const noexcept
{
    const Rect moved{
        static_cast<std::uint16_t>( std::min<std::uint32_t>( area_.x + rect.x, 0xFFFF ) ),
        static_cast<std::uint16_t>( std::min<std::uint32_t>( area_.y + rect.y, 0xFFFF ) ),
        rect.width, rect.height
    };
    return intersect( moved, area_ );
}

std::uint16_t View::put( std::uint16_t x, std::uint16_t y, std::string_view text,
                         const Style &style ) noexcept
{
    if( x >= area_.width || y >= area_.height ) {
        return x;
    }
    const std::uint16_t after =
        screen_->put_within( area_, static_cast<std::uint16_t>( area_.x + x ),
                             static_cast<std::uint16_t>( area_.y + y ), text, style );
    return static_cast<std::uint16_t>( after - area_.x );
}

std::uint16_t View::put( std::uint16_t x, std::uint16_t y, char32_t code,
                         const Style &style ) noexcept
{
    if( x >= area_.width || y >= area_.height ) {
        return x;
    }
    const std::uint16_t after =
        screen_->put_within( area_, static_cast<std::uint16_t>( area_.x + x ),
                             static_cast<std::uint16_t>( area_.y + y ), code, style );
    return static_cast<std::uint16_t>( after - area_.x );
}

void View::fill( Rect rect, char32_t code, const Style &style ) noexcept
{
    screen_->fill( absolute( rect ), code, style );
}

void View::box( const Style &style ) noexcept
{
    screen_->box( area_, style );
}

View View::view( Rect rect ) noexcept
{
    return View{ *screen_, absolute( rect ) };
}

} // namespace sq::tui
