#include <squared/messaging/name_table.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>

namespace sq::messaging
{

NameTable::NameTable( std::span<std::byte> storage ) noexcept
    : storage_( storage )
{}

NameTable::Entry NameTable::entry( std::size_t index ) const noexcept
{
    Entry out{};
    std::memcpy( &out, storage_.data() + index * sizeof( Entry ), sizeof( Entry ) );
    return out;
}

std::size_t NameTable::lower_bound( std::uint64_t id ) const noexcept
{
    std::size_t low = 0;
    std::size_t high = count_;
    while( low < high ) {
        const std::size_t middle = low + ( high - low ) / 2;
        if( entry( middle ).id < id ) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

std::size_t NameTable::used() const noexcept
{
    return count_ * sizeof( Entry ) + text_used_;
}

NameTable::AddResult NameTable::add( std::string_view text ) noexcept
{
    if( !valid_name( text ) ) {
        return AddResult::invalid;
    }
    return insert( name_hash( text ), text );
}

NameTable::AddResult NameTable::insert( std::uint64_t id, std::string_view text ) noexcept
{
    const std::size_t at = lower_bound( id );
    if( at < count_ && entry( at ).id == id ) {
        const Entry found = entry( at );
        const std::string_view known(
            reinterpret_cast<const char *>( storage_.data() + found.offset ), found.length );
        return known == text ? AddResult::known : AddResult::collision;
    }
    if( used() + sizeof( Entry ) + text.size() > storage_.size() ) {
        return AddResult::full;
    }
    // The text, at the end of what is free; then the entries after `at` move up one.
    text_used_ += static_cast<std::uint32_t>( text.size() );
    const auto offset = static_cast<std::uint32_t>( storage_.size() - text_used_ );
    std::memcpy( storage_.data() + offset, text.data(), text.size() );
    std::byte *const slot = storage_.data() + at * sizeof( Entry );
    std::memmove( slot + sizeof( Entry ), slot, ( count_ - at ) * sizeof( Entry ) );
    const Entry added{
        .id = id, .offset = offset, .length = static_cast<std::uint16_t>( text.size() ), .spare = 0
    };
    std::memcpy( slot, &added, sizeof( Entry ) );
    ++count_;
    return AddResult::added;
}

std::string_view NameTable::text( Name name ) const noexcept
{
    const std::size_t at = lower_bound( name.id );
    if( at == count_ || entry( at ).id != name.id ) {
        return {};
    }
    const Entry found = entry( at );
    return { reinterpret_cast<const char *>( storage_.data() + found.offset ), found.length };
}

std::size_t NameTable::format( Address address, std::span<char> out ) const noexcept
{
    char *write = out.data();
    char *const end = out.data() + out.size();
    const auto put = [&]( std::string_view part ) {
        if( write == nullptr || static_cast<std::size_t>( end - write ) < part.size() ) {
            write = nullptr;
            return;
        }
        std::memcpy( write, part.data(), part.size() );
        write += part.size();
    };
    std::array<char, 16> uid_digits{};
    std::string_view uid;
    if( address.uid != 0 ) {
        const auto result =
            std::to_chars( uid_digits.data(), uid_digits.data() + uid_digits.size(), address.uid );
        uid = std::string_view( uid_digits.data(),
                                static_cast<std::size_t>( result.ptr - uid_digits.data() ) );
    }
    const std::string_view name = text( address.name );
    if( name.empty() ) {
        std::array<char, 17> hex{ '#' };
        for( std::size_t i = 0; i < 16; ++i ) {
            hex[1 + i] = "0123456789abcdef"[( address.name.id >> ( 60 - 4 * i ) ) & 0xF];
        }
        put( std::string_view( hex.data(), hex.size() ) );
    } else {
        put( name.substr( 0, std::min( name.find( '.' ), name.size() ) ) );
    }
    if( !uid.empty() ) {
        put( "[" );
        put( uid );
        put( "]" );
    }
    if( !name.empty() && name.find( '.' ) != std::string_view::npos ) {
        put( name.substr( name.find( '.' ) ) );
    }
    return write == nullptr ? 0 : static_cast<std::size_t>( write - out.data() );
}

} // namespace sq::messaging
