// Names, addresses and the name table (Telegraph v2, 6.1): the hash, the text
// form both ways, registration and its failures, and no allocation anywhere.

#include <squared/messaging/address.hpp>
#include <squared/messaging/name.hpp>
#include <squared/messaging/name_table.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>
#include <string_view>
#include <vector>

namespace
{
std::size_t allocations = 0;
} // namespace

void *operator new( std::size_t size )
{
    ++allocations;
    void *p = std::malloc( size == 0 ? 1 : size ); // NOLINT(cppcoreguidelines-no-malloc)
    if( p == nullptr ) {
        std::abort();
    }
    return p;
}
void operator delete( void *p ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}
void operator delete( void *p, std::size_t /*size*/ ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}

namespace sq::messaging
{
/// The test's way to make a collision: no two real names are known to share
/// a 64-bit FNV-1a id.
struct NameTableProbe
{
        static NameTable::AddResult insert( NameTable &table, std::uint64_t id,
                                            std::string_view text )
        {
            return table.insert( id, text );
        }
};
} // namespace sq::messaging

using namespace sq::messaging;
using Result = NameTable::AddResult;

// The published FNV-1a 64 test vectors, at compile time.
static_assert( name_hash( "" ) == 0xcbf29ce484222325ULL );
static_assert( name_hash( "a" ) == 0xaf63dc4c8601ec8cULL );
static_assert( name_hash( "foobar" ) == 0x85944171f73967e8ULL );
// In pieces, the same as whole.
static_assert( name_hash( ".name", name_hash( "item" ) ) == name_hash( "item.name" ) );
// A literal is hashed at compile time.
static_assert( Name( "player.hp" ).id == name_hash( "player.hp" ) );
static_assert( Name( "player.hp" ) == Name::of( "player.hp" ) );
static_assert( valid_name( "player.hp" ) && valid_name( "a" ) && valid_name( "item_2.x_y" ) );
static_assert( !valid_name( "" ) && !valid_name( "Player" ) && !valid_name( "a..b" ) &&
               !valid_name( ".a" ) && !valid_name( "a." ) && !valid_name( "a b" ) &&
               !valid_name( "a-b" ) && !valid_name( "item[1]" ) );

namespace
{

std::string written( const NameTable &table, Address address )
{
    std::array<char, 300> out{};
    const std::size_t length = table.format( address, out );
    return { out.data(), length };
}

} // namespace

int main()
{
    // ---- parsing, with no table ----------------------------------------------------
    assert( ( parse_address( "player.hp" ) == ( Address{ "player.hp", 0 } ) ) );
    assert( ( parse_address( "item[17].name" ) == ( Address{ "item.name", 17 } ) ) );
    assert( ( parse_address( "inventory[20]" ) == ( Address{ "inventory", 20 } ) ) );
    assert( parse_address( "item[17].contents.count" ) ==
            ( Address{ "item.contents.count", 17 } ) );
    assert( parse_address( "item[0].name" ) ==
            ( Address{ "item.name", 0 } ) ); // not canonical, read
    assert(
        ( parse_address( "item[4294967295].name" ) == ( Address{ "item.name", 4294967295U } ) ) );
    assert( parse_address( "#0123456789abcdef" ) ==
            ( Address{ Name::from_id( 0x0123456789abcdefULL ), 0 } ) );
    assert( parse_address( "#0123456789ABCDEF[5]" ) ==
            ( Address{ Name::from_id( 0x0123456789abcdefULL ), 5 } ) );
    for( const std::string_view bad :
         { "", "Player.hp", "a..b", "item[17]x", "item[17]", "item[4294967296].name", "item[].name",
           "item[1][2]", ".a", "a.", "item.name[3]", "item[-1].name", "#123", "#0123456789abcdefg",
           "#0123456789abcdef[1]x", "item[ 1].name" } ) {
        if( bad == "item[17]" ) {
            continue; // a one-segment name with a uid: valid (see inventory[20])
        }
        assert( !parse_address( bad ) );
    }

    // ---- registering ---------------------------------------------------------------
    std::array<std::byte, 8192> storage{};
    NameTable table{ storage };
    assert( table.size() == 0 && table.used() == 0 && table.capacity() == 8192 );
    assert( table.add( "player.hp" ) == Result::added );
    assert( table.add( "player.hp" ) == Result::known );
    assert( table.add( "Player.HP" ) == Result::invalid );
    assert( table.add( "item.name" ) == Result::added );
    assert( table.add( "inventory" ) == Result::added );
    assert( table.size() == 3 );
    assert( table.used() == 3 * 16 + 9 + 9 + 9 );
    assert( table.text( "player.hp" ) == "player.hp" );
    assert( table.text( "item.name" ) == "item.name" );
    assert( table.text( "no.such" ).empty() );
    // Another name with player.hp's id: caught, and the first one kept.
    assert( NameTableProbe::insert( table, Name( "player.hp" ).id, "player.mp" ) ==
            Result::collision );
    assert( table.text( "player.hp" ) == "player.hp" && table.size() == 3 );

    // Many names, in no order: every one found again (the entries stay sorted).
    std::vector<std::string> many;
    for( int i = 0; i < 200; ++i ) {
        many.push_back( "list.row_" + std::to_string( ( i * 7919 ) % 1000 ) );
    }
    for( const std::string &name : many ) {
        const Result result = table.add( name );
        assert( result == Result::added || result == Result::known );
    }
    for( const std::string &name : many ) {
        assert( table.text( Name::of( name ) ) == name );
    }
    assert( table.text( "player.hp" ) == "player.hp" );

    // Full: refused whole, and nothing half-written.
    {
        std::array<std::byte, 40> small{};
        NameTable tiny{ small };
        assert( tiny.add( "abc" ) == Result::added );   // 16 + 3
        assert( tiny.add( "abcdef" ) == Result::full ); // 16 + 6 more is 41
        assert( tiny.add( "abcde" ) == Result::added ); // exactly 40
        assert( tiny.used() == 40 && tiny.size() == 2 );
        assert( tiny.text( "abc" ) == "abc" && tiny.text( "abcde" ) == "abcde" );
        assert( tiny.add( "x" ) == Result::full );
    }

    // ---- writing, and reading back ---------------------------------------------------
    assert( ( written( table, { "player.hp", 0 } ) == "player.hp" ) );
    assert( ( written( table, { "item.name", 17 } ) == "item[17].name" ) );
    assert( ( written( table, { "inventory", 20 } ) == "inventory[20]" ) );
    {
        std::array<char, 17> hex{};
        std::snprintf( hex.data(), hex.size(), "%016llx",
                       static_cast<unsigned long long>( Name( "not.registered" ).id ) );
        const std::string expected = "#" + std::string( hex.data() ) + "[3]";
        assert( ( written( table, Address{ "not.registered", 3 } ) == expected ) );
    }
    for( const Address address :
         { Address{ "player.hp", 0 }, Address{ "item.name", 17 },
           Address{ "inventory", 4294967295U }, Address{ "not.registered", 0 },
           Address{ "not.registered", 9 } } ) {
        assert( parse_address( written( table, address ) ) == address );
    }
    {
        std::array<char, 8> short_buffer{};
        assert( table.format( { "item.name", 17 }, short_buffer ) == 0 ); // 13 bytes needed
        std::array<char, 13> exact{};
        assert( ( table.format( { "item.name", 17 }, exact ) == 13 ) );
    }

    // ---- nothing allocates ----------------------------------------------------------
    {
        std::array<std::byte, 1024> more{};
        NameTable fresh{ more };
        std::array<char, 64> out{};
        const std::size_t before = allocations;
        assert( fresh.add( "sound.on" ) == Result::added );
        assert( fresh.text( "sound.on" ) == "sound.on" );
        assert( ( fresh.format( { "sound.on", 2 }, out ) == 11 ) );
        assert( ( parse_address( "sound[2].on" ) == ( Address{ "sound.on", 2 } ) ) );
        assert( allocations == before );
    }

    std::printf( "address: all assertions passed  sizeof(Name)=%zu sizeof(Address)=%zu "
                 "sizeof(NameTable)=%zu; 203 names in %zu of %zu bytes\n",
                 sizeof( Name ), sizeof( Address ), sizeof( NameTable ), table.used(),
                 table.capacity() );
    return 0;
}
