#include <squared/console/scrollback.hpp>

#include <algorithm>
#include <cassert>
#include <cstring>

namespace sq::console
{

// A record is a 4-byte header, the text, and padding to a multiple of 4:
//
//   [length low][length high][level][0][text ... ][pad]
//
// Records never wrap around the end of the storage. When one does not fit
// above the tail, the upper records end there (end_), and new ones start again
// at 0, dropping the oldest until there is room below head_. Every line is
// therefore contiguous, and handed out as a string_view into the storage.

namespace
{

constexpr std::uint32_t k_header = 4;
constexpr std::uint32_t k_longest = 0xFFFF;

constexpr std::uint32_t padded( std::uint32_t bytes ) noexcept
{
    return ( bytes + 3U ) & ~3U;
}

} // namespace

Scrollback::Scrollback( std::span<char> storage ) noexcept
    : storage_( storage )
    , capacity_( static_cast<std::uint32_t>( std::min<std::size_t>( storage.size(), 0x7FFFFFFC ) &
                                             ~std::size_t{ 3 } ) )
{
    assert( capacity_ >= 64 && "Scrollback: give it at least 64 bytes" );
}

std::size_t Scrollback::longest_line() const noexcept
{
    return std::min( k_longest, capacity_ - k_header );
}

void Scrollback::clear() noexcept
{
    count_ = 0;
    head_ = tail_ = end_ = 0;
    wrapped_ = false;
}

std::uint32_t Scrollback::record_size( std::uint32_t at ) const noexcept
{
    std::uint16_t length = 0;
    std::memcpy( &length, storage_.data() + at, sizeof( length ) );
    return padded( k_header + length );
}

std::uint32_t Scrollback::next( std::uint32_t at ) const noexcept
{
    at += record_size( at );
    return ( wrapped_ && at == end_ ) ? 0 : at;
}

void Scrollback::drop_oldest() noexcept
{
    head_ += record_size( head_ );
    --count_;
    ++dropped_;
    if( wrapped_ && head_ == end_ ) {
        head_ = 0;
        wrapped_ = false;
    }
    if( count_ == 0 ) {
        clear();
    }
}

void Scrollback::push( Level level, std::string_view text ) noexcept
{
    if( !text.empty() && text.back() == '\n' ) {
        text.remove_suffix( 1 );
    }
    for( ;; ) {
        const auto newline = text.find( '\n' );
        push_line( level, text.substr( 0, newline ) );
        if( newline == std::string_view::npos ) {
            return;
        }
        text.remove_prefix( newline + 1 );
    }
}

void Scrollback::push_line( Level level, std::string_view text ) noexcept
{
    const auto length = static_cast<std::uint32_t>( std::min( text.size(), longest_line() ) );
    const std::uint32_t size = padded( k_header + length );
    for( ;; ) {
        if( count_ == 0 ) {
            clear();
        }
        if( !wrapped_ ) {
            if( capacity_ - tail_ >= size ) {
                break;
            }
            // No room above: the upper records end here, and the new one
            // starts at 0, below the oldest.
            end_ = tail_;
            tail_ = 0;
            wrapped_ = true;
        }
        if( head_ - tail_ >= size ) {
            break;
        }
        drop_oldest();
    }
    char *record = storage_.data() + tail_;
    const auto length16 = static_cast<std::uint16_t>( length );
    std::memcpy( record, &length16, sizeof( length16 ) );
    record[2] = static_cast<char>( level );
    record[3] = 0;
    std::memcpy( record + k_header, text.data(), length );
    tail_ += size;
    ++count_;
}

Scrollback::Iterator Scrollback::begin() const noexcept
{
    return count_ == 0 ? end() : Iterator( this, head_, count_ );
}

Scrollback::Iterator Scrollback::last( std::size_t count ) const noexcept
{
    Iterator it = begin();
    for( std::size_t skip = count_ > count ? count_ - count : 0; skip > 0; --skip ) {
        ++it;
    }
    return it;
}

Line Scrollback::Iterator::operator*() const noexcept
{
    const char *record = owner_->storage_.data() + at_;
    std::uint16_t length = 0;
    std::memcpy( &length, record, sizeof( length ) );
    return { std::string_view( record + k_header, length ),
             static_cast<Level>( static_cast<unsigned char>( record[2] ) ) };
}

Scrollback::Iterator &Scrollback::Iterator::operator++() noexcept
{
    --remaining_;
    if( remaining_ == 0 ) {
        *this = {};
    } else {
        at_ = owner_->next( at_ );
    }
    return *this;
}

} // namespace sq::console
