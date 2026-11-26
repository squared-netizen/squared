// sq.data's zstd: round trips, several frames, the limit, and what is not
// a frame; and <zstd.h> reachable through the library, for what wants more.

#include <zstd.h>

#include <cassert>
#include <cstdio>

import std;
import sq.data;

namespace
{

std::span<const std::byte> bytes( std::string_view s )
{
    return std::as_bytes( std::span( s.data(), s.size() ) );
}

std::string text( const std::vector<std::byte> &b )
{
    return { reinterpret_cast<const char *>( b.data() ), b.size() };
}

} // namespace

int main()
{
    // A round trip, and smaller for being repetitive.
    std::string plain;
    for( int i = 0; i < 20000; ++i ) {
        plain += "squared settings " + std::to_string( i % 7 ) + "\n";
    }
    const sq::data::ZstdResult packed = sq::data::compress( bytes( plain ) );
    assert( packed && packed.bytes.size() < plain.size() / 10 );
    const sq::data::ZstdResult back = sq::data::decompress( packed.bytes );
    assert( back && text( back.bytes ) == plain );

    // Levels out of range are clamped, not refused; nothing compresses too.
    assert( sq::data::compress( bytes( plain ), 1000 ) );
    assert( sq::data::compress( bytes( plain ), -1000000 ) );
    const sq::data::ZstdResult none = sq::data::compress( {} );
    assert( none && !none.bytes.empty() );
    const sq::data::ZstdResult empty = sq::data::decompress( none.bytes );
    assert( empty && empty.bytes.empty() );

    // Two frames, one after the other: one output.
    std::vector<std::byte> two = sq::data::compress( bytes( "one " ) ).bytes;
    const std::vector<std::byte> second = sq::data::compress( bytes( "two" ) ).bytes;
    two.insert( two.end(), second.begin(), second.end() );
    const sq::data::ZstdResult both = sq::data::decompress( two );
    assert( both && text( both.bytes ) == "one two" );

    // The limit: exactly it is fine, one byte more is not.
    assert( sq::data::decompress( packed.bytes, plain.size() ) );
    const sq::data::ZstdResult over = sq::data::decompress( packed.bytes, plain.size() - 1 );
    assert( !over && over.bytes.empty() && over.error.find( "more than" ) != std::string::npos );

    // Truncated, and not zstd at all.
    const std::span<const std::byte> cut( packed.bytes.data(), packed.bytes.size() / 2 );
    assert( !sq::data::decompress( cut ) );
    assert( !sq::data::decompress( bytes( "not a zstd frame" ) ) );
    assert( !sq::data::decompress( {} ) );

    // The C API, from the same library.
    assert( ZSTD_versionNumber() == ZSTD_VERSION_NUMBER && ZSTD_VERSION_NUMBER == 10507 );

    std::printf( "zstd: all assertions passed\n" );
    return 0;
}
