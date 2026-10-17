#include <squared/console/input_line.hpp>

#include <algorithm>
#include <cstring>

namespace sq::console
{

namespace
{

bool continuation( char c ) noexcept
{
    return ( static_cast<unsigned char>( c ) & 0xC0U ) == 0x80U;
}

// The longest prefix of `text`, at most `room` bytes, that ends on a character
// boundary.
std::size_t fit( std::string_view text, std::size_t room ) noexcept
{
    if( text.size() <= room ) {
        return text.size();
    }
    std::size_t n = room;
    while( n > 0 && continuation( text[n] ) ) {
        --n;
    }
    return n;
}

} // namespace

InputLine::InputLine() noexcept
    : history_( history_storage_ )
{}

void InputLine::insert( std::string_view utf8 ) noexcept
{
    for( std::size_t at = 0; at < utf8.size(); ) {
        // One run of printable bytes at a time: control characters are dropped.
        std::size_t run = at;
        while( run < utf8.size() && static_cast<unsigned char>( utf8[run] ) >= 0x20U &&
               utf8[run] != 0x7F ) {
            ++run;
        }
        const std::string_view piece = utf8.substr( at, run - at );
        const std::size_t n = fit( piece, buffer_.size() - length_ );
        std::memmove( buffer_.data() + cursor_ + n, buffer_.data() + cursor_, length_ - cursor_ );
        std::memcpy( buffer_.data() + cursor_, piece.data(), n );
        length_ += n;
        cursor_ += n;
        if( n < piece.size() ) {
            return; // full
        }
        at = run + 1;
    }
}

void InputLine::erase_before() noexcept
{
    if( cursor_ == 0 ) {
        return;
    }
    std::size_t start = cursor_ - 1;
    while( start > 0 && continuation( buffer_[start] ) ) {
        --start;
    }
    std::memmove( buffer_.data() + start, buffer_.data() + cursor_, length_ - cursor_ );
    length_ -= cursor_ - start;
    cursor_ = start;
}

void InputLine::erase_after() noexcept
{
    if( cursor_ == length_ ) {
        return;
    }
    right();
    erase_before();
}

void InputLine::left() noexcept
{
    while( cursor_ > 0 ) {
        --cursor_;
        if( !continuation( buffer_[cursor_] ) ) {
            return;
        }
    }
}

void InputLine::right() noexcept
{
    if( cursor_ < length_ ) {
        ++cursor_;
        while( cursor_ < length_ && continuation( buffer_[cursor_] ) ) {
            ++cursor_;
        }
    }
}

void InputLine::set( std::string_view utf8 ) noexcept
{
    length_ = 0;
    cursor_ = 0;
    insert( utf8 );
}

void InputLine::clear() noexcept
{
    length_ = 0;
    cursor_ = 0;
    browsing_ = 0;
}

void InputLine::previous() noexcept
{
    if( browsing_ == history_.size() ) {
        return;
    }
    if( browsing_ == 0 ) {
        std::memcpy( draft_.data(), buffer_.data(), length_ );
        draft_length_ = length_;
    }
    ++browsing_;
    const std::size_t keep = browsing_;
    set( ( *history_.last( browsing_ ) ).text );
    browsing_ = keep;
}

void InputLine::next() noexcept
{
    if( browsing_ == 0 ) {
        return;
    }
    --browsing_;
    const std::size_t keep = browsing_;
    if( browsing_ == 0 ) {
        set( { draft_.data(), draft_length_ } );
    } else {
        set( ( *history_.last( browsing_ ) ).text );
    }
    browsing_ = keep;
}

void InputLine::remember() noexcept
{
    browsing_ = 0;
    if( length_ == 0 ) {
        return;
    }
    if( !history_.empty() && ( *history_.last( 1 ) ).text == text() ) {
        return;
    }
    history_.push( Level::info, text() );
}

} // namespace sq::console
