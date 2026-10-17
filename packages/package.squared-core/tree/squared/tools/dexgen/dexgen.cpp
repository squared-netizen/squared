// dexgen: writes the shim dex, and prints any dex it is given.
//
//   dexgen shims.dex            write the shims below to shims.dex
//   dexgen --dump shims.dex     print its header, checksums, tables, classes,
//                               methods and every instruction
//
// A shim is a class Android needs to call into us: an activity in another
// process, a service. The rule for every shim (docs/developer/shims.md):
//
//   All behaviour stays in C++. The dex only tells Android "these classes
//   exist, and their methods live in that library."
//
// So this program cannot express logic. A shim method is one of two kinds: a
// constructor that calls its parent's (two instructions), or a native
// declaration (no instructions; C++ registers it with RegisterNatives). A shim
// that needs more is the wrong shim.
//
// Written from the published format (source.android.com/docs/core/runtime/
// dex-format), version 035, which every Android release from 5.0 accepts.
// No javac, no d8, no JDK. This is a maintainer tool, built and run on the
// host; the standard library's containers are used freely, as the framework
// rules allow outside resident code.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace
{

// --- the shims -----------------------------------------------------------------

enum class MethodKind : std::uint8_t {
    constructor, // public <init>()V that calls the parent's <init>()V
    native       // public native, bound by RegisterNatives
};

struct ShimMethod
{
        MethodKind kind;
        std::string_view name;                    // ignored for a constructor
        std::string_view returns;                 // a type descriptor, such as "V"
        std::vector<std::string_view> parameters; // type descriptors
};

struct ShimClass
{
        std::string_view descriptor; // "Ldev/squared/ConsoleActivity;"
        std::string_view parent;     // "Landroid/app/NativeActivity;"
        std::vector<ShimMethod> methods;
};

// Every shim squared ships. The names do not depend on the application's
// package, so this dex is the same for every app, and is generated once.
//
// ConsoleActivity: the monitor process (CONSOLE=monitor). Only a constructor:
// NativeActivity reads android.app.lib_name from the activity's own manifest
// entry, so a second entry with its own process and library needs no code.
const std::vector<ShimClass> &shims()
{
    static const std::vector<ShimClass> classes = {
        { "Ldev/squared/ConsoleActivity;",
          "Landroid/app/NativeActivity;",
          { { MethodKind::constructor, "<init>", "V", {} } } },
#ifdef DEXGEN_SELF_TEST
        // Built only by tools/dexgen.sh --self-test: exercises native methods,
        // parameter lists and a second class, which no shipped shim uses yet.
        { "Ldev/squared/TestService;",
          "Landroid/app/Service;",
          { { MethodKind::constructor, "<init>", "V", {} },
            { MethodKind::native, "onStartCommand", "I", { "Landroid/content/Intent;", "I", "I" } },
            { MethodKind::native,
              "onBind",
              "Landroid/os/IBinder;",
              { "Landroid/content/Intent;" } },
            { MethodKind::native, "onDestroy", "V", {} } } },
#endif
    };
    return classes;
}

// --- byte helpers ---------------------------------------------------------------

constexpr std::uint32_t k_no_index = 0xFFFFFFFFU;
constexpr std::uint32_t k_acc_public = 0x0001;
constexpr std::uint32_t k_acc_native = 0x0100;
constexpr std::uint32_t k_acc_constructor = 0x10000;
constexpr std::size_t k_header_size = 0x70;

struct Bytes
{
        std::vector<std::uint8_t> data;

        [[nodiscard]] std::size_t size() const { return data.size(); }
        void u8( std::uint32_t value ) { data.push_back( static_cast<std::uint8_t>( value ) ); }
        void u16( std::uint32_t value )
        {
            u8( value & 0xFFU );
            u8( ( value >> 8U ) & 0xFFU );
        }
        void u32( std::uint32_t value )
        {
            u16( value & 0xFFFFU );
            u16( value >> 16U );
        }
        void uleb( std::uint32_t value )
        {
            do {
                std::uint32_t byte = value & 0x7FU;
                value >>= 7U;
                if( value != 0 ) {
                    byte |= 0x80U;
                }
                u8( byte );
            } while( value != 0 );
        }
        void align4()
        {
            while( data.size() % 4 != 0 ) {
                u8( 0 );
            }
        }
        void put_u32_at( std::size_t at, std::uint32_t value )
        {
            for( std::size_t i = 0; i < 4; ++i ) {
                data[at + i] = static_cast<std::uint8_t>( value >> ( 8U * i ) );
            }
        }
};

std::uint32_t adler32( const std::uint8_t *data, std::size_t size )
{
    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for( std::size_t i = 0; i < size; ++i ) {
        a = ( a + data[i] ) % 65521U;
        b = ( b + a ) % 65521U;
    }
    return ( b << 16U ) | a;
}

std::array<std::uint8_t, 20> sha1( const std::uint8_t *data, std::size_t size )
{
    std::uint32_t h[5] = { 0x67452301U, 0xEFCDAB89U, 0x98BADCFEU, 0x10325476U, 0xC3D2E1F0U };
    std::vector<std::uint8_t> message( data, data + size );
    const std::uint64_t bits = static_cast<std::uint64_t>( size ) * 8U;
    message.push_back( 0x80 );
    while( message.size() % 64 != 56 ) {
        message.push_back( 0 );
    }
    for( int i = 7; i >= 0; --i ) {
        message.push_back( static_cast<std::uint8_t>( bits >> ( 8 * i ) ) );
    }
    const auto rotl = []( std::uint32_t x, unsigned n ) {
        return ( x << n ) | ( x >> ( 32U - n ) );
    };
    for( std::size_t block = 0; block < message.size(); block += 64 ) {
        std::uint32_t w[80];
        for( std::size_t i = 0; i < 16; ++i ) {
            const std::uint8_t *p = &message[block + i * 4];
            w[i] = ( std::uint32_t{ p[0] } << 24U ) | ( std::uint32_t{ p[1] } << 16U ) |
                   ( std::uint32_t{ p[2] } << 8U ) | std::uint32_t{ p[3] };
        }
        for( std::size_t i = 16; i < 80; ++i ) {
            w[i] = rotl( w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1 );
        }
        std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for( std::size_t i = 0; i < 80; ++i ) {
            std::uint32_t f = 0;
            std::uint32_t k = 0;
            if( i < 20 ) {
                f = ( b & c ) | ( ~b & d );
                k = 0x5A827999U;
            } else if( i < 40 ) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1U;
            } else if( i < 60 ) {
                f = ( b & c ) | ( b & d ) | ( c & d );
                k = 0x8F1BBCDCU;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6U;
            }
            const std::uint32_t t = rotl( a, 5 ) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotl( b, 30 );
            b = a;
            a = t;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
    }
    std::array<std::uint8_t, 20> out{};
    for( std::size_t i = 0; i < 20; ++i ) {
        out[i] = static_cast<std::uint8_t>( h[i / 4] >> ( 24U - 8U * ( i % 4 ) ) );
    }
    return out;
}

// --- writing ----------------------------------------------------------------------

char shorty_of( std::string_view descriptor )
{
    return descriptor[0] == '[' ? 'L' : descriptor[0];
}

struct Proto
{
        std::string shorty;
        std::string_view returns;
        std::vector<std::string_view> parameters;
};

struct MethodRef
{
        std::string_view owner;
        std::string_view name;
        std::size_t proto; // index into the unsorted proto list
};

std::vector<std::uint8_t> write_dex( const std::vector<ShimClass> &classes )
{
    // Everything the file names, collected first; indices come from sorting.
    std::vector<std::string> strings;
    std::vector<Proto> protos;
    std::vector<MethodRef> refs;
    const auto add_string = [&]( std::string_view text ) { strings.emplace_back( text ); };
    const auto proto_of = [&]( std::string_view returns,
                               const std::vector<std::string_view> &parameters ) {
        Proto proto{ std::string( 1, shorty_of( returns ) ), returns, parameters };
        for( const auto parameter : parameters ) {
            proto.shorty += shorty_of( parameter );
        }
        for( std::size_t i = 0; i < protos.size(); ++i ) {
            if( protos[i].returns == returns && protos[i].parameters == parameters ) {
                return i;
            }
        }
        protos.push_back( proto );
        add_string( proto.shorty );
        add_string( returns );
        for( const auto parameter : parameters ) {
            add_string( parameter );
        }
        return protos.size() - 1;
    };
    for( const auto &shim : classes ) {
        add_string( shim.descriptor );
        add_string( shim.parent );
        for( const auto &method : shim.methods ) {
            const bool ctor = method.kind == MethodKind::constructor;
            const std::string_view name = ctor ? "<init>" : method.name;
            const std::size_t proto =
                ctor ? proto_of( "V", {} ) : proto_of( method.returns, method.parameters );
            add_string( name );
            refs.push_back( { shim.descriptor, name, proto } );
            if( ctor ) {
                refs.push_back( { shim.parent, "<init>", proto } );
            }
        }
    }

    // Strings: sorted by code point. Every name here is ASCII, so byte order
    // is code-point order.
    std::sort( strings.begin(), strings.end() );
    strings.erase( std::unique( strings.begin(), strings.end() ), strings.end() );
    const auto string_index = [&]( std::string_view text ) {
        return static_cast<std::uint32_t>(
            std::lower_bound( strings.begin(), strings.end(), text ) - strings.begin() );
    };

    // Types: every string that is a descriptor, sorted by string index.
    std::vector<std::uint32_t> types;
    const auto note_type = [&]( std::string_view descriptor ) {
        types.push_back( string_index( descriptor ) );
    };
    for( const auto &shim : classes ) {
        note_type( shim.descriptor );
        note_type( shim.parent );
    }
    for( const auto &proto : protos ) {
        note_type( proto.returns );
        for( const auto parameter : proto.parameters ) {
            note_type( parameter );
        }
    }
    std::sort( types.begin(), types.end() );
    types.erase( std::unique( types.begin(), types.end() ), types.end() );
    const auto type_index = [&]( std::string_view descriptor ) {
        return static_cast<std::uint32_t>(
            std::lower_bound( types.begin(), types.end(), string_index( descriptor ) ) -
            types.begin() );
    };

    // Protos: by return type, then by parameter types.
    using ProtoKey = std::pair<std::uint32_t, std::vector<std::uint32_t>>;
    std::vector<std::pair<ProtoKey, std::size_t>> proto_order;
    for( std::size_t i = 0; i < protos.size(); ++i ) {
        ProtoKey key{ type_index( protos[i].returns ), {} };
        for( const auto parameter : protos[i].parameters ) {
            key.second.push_back( type_index( parameter ) );
        }
        proto_order.emplace_back( key, i );
    }
    std::sort( proto_order.begin(), proto_order.end() );
    std::vector<std::uint32_t> proto_index( protos.size() );
    for( std::size_t i = 0; i < proto_order.size(); ++i ) {
        proto_index[proto_order[i].second] = static_cast<std::uint32_t>( i );
    }

    // Methods: by class, then name, then proto.
    using MethodKey = std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>;
    std::vector<MethodKey> methods;
    methods.reserve( refs.size() );
    for( const auto &ref : refs ) {
        methods.emplace_back( type_index( ref.owner ), string_index( ref.name ),
                              proto_index[ref.proto] );
    }
    std::sort( methods.begin(), methods.end() );
    methods.erase( std::unique( methods.begin(), methods.end() ), methods.end() );
    const auto method_index = [&]( std::string_view owner, std::string_view name,
                                   std::size_t proto ) {
        const MethodKey key{ type_index( owner ), string_index( name ), proto_index[proto] };
        return static_cast<std::uint32_t>( std::lower_bound( methods.begin(), methods.end(), key ) -
                                           methods.begin() );
    };

    // Class definitions, by descriptor. A shim's parent is a framework class,
    // never one defined here, so no ordering constraint applies.
    std::vector<const ShimClass *> defs;
    defs.reserve( classes.size() );
    for( const auto &shim : classes ) {
        defs.push_back( &shim );
    }
    std::sort( defs.begin(), defs.end(), []( const ShimClass *a, const ShimClass *b ) {
        return a->descriptor < b->descriptor;
    } );

    // Layout. The id tables follow the header in a fixed order, so the data
    // section's start is known before any of it is written.
    const std::size_t string_ids_off = k_header_size;
    const std::size_t type_ids_off = string_ids_off + 4 * strings.size();
    const std::size_t proto_ids_off = type_ids_off + 4 * types.size();
    const std::size_t method_ids_off = proto_ids_off + 12 * protos.size();
    const std::size_t class_defs_off = method_ids_off + 8 * methods.size();
    std::size_t data_off = class_defs_off + 32 * defs.size();
    data_off = ( data_off + 3 ) & ~std::size_t{ 3 };

    Bytes data; // positions in it are relative to data_off
    const auto at = [&]() { return static_cast<std::uint32_t>( data_off + data.size() ); };

    // Code items: one per constructor, 4-aligned.
    std::map<std::pair<const ShimClass *, std::size_t>, std::uint32_t> code_offsets;
    std::size_t code_items = 0;
    const std::size_t code_start = data.size();
    for( const ShimClass *shim : defs ) {
        for( std::size_t m = 0; m < shim->methods.size(); ++m ) {
            if( shim->methods[m].kind != MethodKind::constructor ) {
                continue;
            }
            data.align4();
            code_offsets[{ shim, m }] = at();
            ++code_items;
            data.u16( 1 ); // registers: this
            data.u16( 1 ); // ins: this
            data.u16( 1 ); // outs: the parent constructor's this
            data.u16( 0 ); // tries
            data.u32( 0 ); // debug info: none
            data.u32( 4 ); // code units
            // invoke-direct {v0}, parent.<init>()V   (format 35c: A=1, G=0)
            data.u16( 0x1070 );
            data.u16( method_index( shim->parent, "<init>", proto_of( "V", {} ) ) );
            data.u16( 0x0000 );
            data.u16( 0x000E ); // return-void
        }
    }

    // Type lists: one per distinct non-empty parameter list, 4-aligned.
    std::map<std::vector<std::uint32_t>, std::uint32_t> list_offsets;
    std::size_t type_lists = 0;
    std::size_t list_start = 0;
    for( const auto &entry : proto_order ) {
        const auto &params = entry.first.second;
        if( params.empty() || list_offsets.count( params ) != 0 ) {
            continue;
        }
        data.align4();
        if( type_lists == 0 ) {
            list_start = data.size();
        }
        list_offsets[params] = at();
        ++type_lists;
        data.u32( static_cast<std::uint32_t>( params.size() ) );
        for( const auto type : params ) {
            data.u16( type );
        }
    }

    // String data, in index order.
    std::vector<std::uint32_t> string_offsets;
    const std::size_t strings_start = data.size();
    for( const auto &text : strings ) {
        string_offsets.push_back( at() );
        data.uleb( static_cast<std::uint32_t>( text.size() ) );
        for( const char c : text ) {
            data.u8( static_cast<std::uint8_t>( c ) );
        }
        data.u8( 0 );
    }

    // Class data: direct methods (constructors), then virtual (natives), each
    // list sorted by method index and encoded as differences.
    std::vector<std::uint32_t> class_data_offsets;
    const std::size_t class_data_start = data.size();
    for( const ShimClass *shim : defs ) {
        struct Entry
        {
                std::uint32_t index;
                std::uint32_t flags;
                std::uint32_t code;
        };
        std::vector<Entry> direct;
        std::vector<Entry> virtuals;
        for( std::size_t m = 0; m < shim->methods.size(); ++m ) {
            const auto &method = shim->methods[m];
            if( method.kind == MethodKind::constructor ) {
                direct.push_back( { method_index( shim->descriptor, "<init>", proto_of( "V", {} ) ),
                                    k_acc_public | k_acc_constructor, code_offsets[{ shim, m }] } );
            } else {
                virtuals.push_back( { method_index( shim->descriptor, method.name,
                                                    proto_of( method.returns, method.parameters ) ),
                                      k_acc_public | k_acc_native, 0 } );
            }
        }
        const auto by_index = []( const Entry &a, const Entry &b ) { return a.index < b.index; };
        std::sort( direct.begin(), direct.end(), by_index );
        std::sort( virtuals.begin(), virtuals.end(), by_index );
        class_data_offsets.push_back( at() );
        data.uleb( 0 ); // static fields
        data.uleb( 0 ); // instance fields
        data.uleb( static_cast<std::uint32_t>( direct.size() ) );
        data.uleb( static_cast<std::uint32_t>( virtuals.size() ) );
        for( const auto *list : { &direct, &virtuals } ) {
            std::uint32_t previous = 0;
            for( const auto &entry : *list ) {
                data.uleb( entry.index - previous );
                data.uleb( entry.flags );
                data.uleb( entry.code );
                previous = entry.index;
            }
        }
    }

    // The map, last, 4-aligned.
    data.align4();
    const std::uint32_t map_off = at();
    struct MapItem
    {
            std::uint16_t type;
            std::uint32_t size;
            std::uint32_t offset;
    };
    std::vector<MapItem> map = {
        { 0x0000, 1, 0 },
        { 0x0001, static_cast<std::uint32_t>( strings.size() ),
          static_cast<std::uint32_t>( string_ids_off ) },
        { 0x0002, static_cast<std::uint32_t>( types.size() ),
          static_cast<std::uint32_t>( type_ids_off ) },
        { 0x0003, static_cast<std::uint32_t>( protos.size() ),
          static_cast<std::uint32_t>( proto_ids_off ) },
        { 0x0005, static_cast<std::uint32_t>( methods.size() ),
          static_cast<std::uint32_t>( method_ids_off ) },
        { 0x0006, static_cast<std::uint32_t>( defs.size() ),
          static_cast<std::uint32_t>( class_defs_off ) },
    };
    if( code_items != 0 ) {
        map.push_back( { 0x2001, static_cast<std::uint32_t>( code_items ),
                         static_cast<std::uint32_t>( data_off + code_start ) } );
    }
    if( type_lists != 0 ) {
        map.push_back( { 0x1001, static_cast<std::uint32_t>( type_lists ),
                         static_cast<std::uint32_t>( data_off + list_start ) } );
    }
    map.push_back( { 0x2002, static_cast<std::uint32_t>( strings.size() ),
                     static_cast<std::uint32_t>( data_off + strings_start ) } );
    map.push_back( { 0x2000, static_cast<std::uint32_t>( defs.size() ),
                     static_cast<std::uint32_t>( data_off + class_data_start ) } );
    map.push_back( { 0x1000, 1, map_off } );
    data.u32( static_cast<std::uint32_t>( map.size() ) );
    for( const auto &item : map ) {
        data.u16( item.type );
        data.u16( 0 );
        data.u32( item.size );
        data.u32( item.offset );
    }

    // The file: header, id tables, padding, data.
    Bytes out;
    for( const char c : std::string_view( "dex\n035\0", 8 ) ) {
        out.u8( static_cast<std::uint8_t>( c ) );
    }
    out.u32( 0 ); // checksum, below
    for( int i = 0; i < 20; ++i ) {
        out.u8( 0 ); // signature, below
    }
    const auto file_size = static_cast<std::uint32_t>( data_off + data.size() );
    out.u32( file_size );
    out.u32( static_cast<std::uint32_t>( k_header_size ) );
    out.u32( 0x12345678U ); // endian tag
    out.u32( 0 );           // link size
    out.u32( 0 );           // link offset
    out.u32( map_off );
    out.u32( static_cast<std::uint32_t>( strings.size() ) );
    out.u32( static_cast<std::uint32_t>( string_ids_off ) );
    out.u32( static_cast<std::uint32_t>( types.size() ) );
    out.u32( static_cast<std::uint32_t>( type_ids_off ) );
    out.u32( static_cast<std::uint32_t>( protos.size() ) );
    out.u32( static_cast<std::uint32_t>( proto_ids_off ) );
    out.u32( 0 ); // fields
    out.u32( 0 );
    out.u32( static_cast<std::uint32_t>( methods.size() ) );
    out.u32( static_cast<std::uint32_t>( method_ids_off ) );
    out.u32( static_cast<std::uint32_t>( defs.size() ) );
    out.u32( static_cast<std::uint32_t>( class_defs_off ) );
    out.u32( static_cast<std::uint32_t>( data.size() ) );
    out.u32( static_cast<std::uint32_t>( data_off ) );

    for( const auto offset : string_offsets ) {
        out.u32( offset );
    }
    for( const auto descriptor : types ) {
        out.u32( descriptor );
    }
    for( const auto &entry : proto_order ) {
        const Proto &proto = protos[entry.second];
        out.u32( string_index( proto.shorty ) );
        out.u32( type_index( proto.returns ) );
        out.u32( proto.parameters.empty() ? 0 : list_offsets[entry.first.second] );
    }
    for( const auto &[owner, name, proto] : methods ) {
        out.u16( owner );
        out.u16( proto );
        out.u32( name );
    }
    for( std::size_t i = 0; i < defs.size(); ++i ) {
        out.u32( type_index( defs[i]->descriptor ) );
        out.u32( k_acc_public );
        out.u32( type_index( defs[i]->parent ) );
        out.u32( 0 );          // interfaces
        out.u32( k_no_index ); // source file
        out.u32( 0 );          // annotations
        out.u32( class_data_offsets[i] );
        out.u32( 0 ); // static values
    }
    while( out.size() < data_off ) {
        out.u8( 0 );
    }
    out.data.insert( out.data.end(), data.data.begin(), data.data.end() );

    // Signature over everything after it; then the checksum over everything
    // after the checksum, signature included.
    const auto signature = sha1( out.data.data() + 32, out.size() - 32 );
    std::copy( signature.begin(), signature.end(), out.data.begin() + 12 );
    out.put_u32_at( 8, adler32( out.data.data() + 12, out.size() - 12 ) );
    return out.data;
}

// --- dumping ----------------------------------------------------------------------

struct Reader
{
        const std::vector<std::uint8_t> &data;
        bool ok = true;

        std::uint32_t u8( std::size_t at )
        {
            if( at >= data.size() ) {
                ok = false;
                return 0;
            }
            return data[at];
        }
        std::uint32_t u16( std::size_t at ) { return u8( at ) | ( u8( at + 1 ) << 8U ); }
        std::uint32_t u32( std::size_t at ) { return u16( at ) | ( u16( at + 2 ) << 16U ); }
        std::uint32_t uleb( std::size_t &at )
        {
            std::uint32_t value = 0;
            for( unsigned shift = 0; shift < 35; shift += 7 ) {
                const std::uint32_t byte = u8( at++ );
                value |= ( byte & 0x7FU ) << shift;
                if( ( byte & 0x80U ) == 0 ) {
                    return value;
                }
            }
            ok = false;
            return value;
        }
        std::string string( std::uint32_t index, std::uint32_t ids_off )
        {
            std::size_t at = u32( ids_off + 4 * index );
            uleb( at );
            std::string text;
            while( ok && u8( at ) != 0 ) {
                text += static_cast<char>( u8( at++ ) );
            }
            return text;
        }
};

std::string flags_text( std::uint32_t flags )
{
    std::string text;
    const std::pair<std::uint32_t, const char *> names[] = {
        { 0x1, "public" }, { 0x2, "private" },  { 0x4, "protected" },  { 0x8, "static" },
        { 0x10, "final" }, { 0x100, "native" }, { 0x400, "abstract" }, { 0x10000, "constructor" }
    };
    for( const auto &[bit, name] : names ) {
        if( ( flags & bit ) != 0 ) {
            text += text.empty() ? "" : " ";
            text += name;
        }
    }
    return text.empty() ? "-" : text;
}

int dump( const std::vector<std::uint8_t> &file )
{
    Reader r{ file };
    if( file.size() < k_header_size || std::memcmp( file.data(), "dex\n", 4 ) != 0 ) {
        std::fprintf( stderr, "dexgen: not a dex file\n" );
        return 1;
    }
    bool good = true;
    const auto check = [&]( bool pass, const char *what ) {
        std::printf( "  %-14s %s\n", what, pass ? "ok" : "WRONG" );
        good = good && pass;
    };
    std::printf( "header\n  %-14s %.3s\n", "version", reinterpret_cast<const char *>( &file[4] ) );
    check( r.u32( 8 ) == adler32( file.data() + 12, file.size() - 12 ), "checksum" );
    const auto signature = sha1( file.data() + 32, file.size() - 32 );
    check( std::equal( signature.begin(), signature.end(), file.begin() + 12 ), "signature" );
    check( r.u32( 32 ) == file.size(), "file size" );
    check( r.u32( 40 ) == 0x12345678U, "endian tag" );

    const std::uint32_t strings = r.u32( 56 ), strings_off = r.u32( 60 );
    const std::uint32_t types = r.u32( 64 ), types_off = r.u32( 68 );
    const std::uint32_t protos = r.u32( 72 ), protos_off = r.u32( 76 );
    const std::uint32_t methods = r.u32( 88 ), methods_off = r.u32( 92 );
    const std::uint32_t classes = r.u32( 96 ), classes_off = r.u32( 100 );
    std::printf( "  %-14s %zu bytes; %u strings, %u types, %u protos, %u methods, %u classes\n",
                 "size", file.size(), strings, types, protos, methods, classes );

    const auto str = [&]( std::uint32_t i ) { return r.string( i, strings_off ); };
    const auto type = [&]( std::uint32_t i ) { return str( r.u32( types_off + 4 * i ) ); };
    const auto proto = [&]( std::uint32_t i ) {
        const std::size_t at = protos_off + 12 * i;
        std::string text = "(";
        const std::uint32_t list = r.u32( at + 8 );
        if( list != 0 ) {
            const std::uint32_t n = r.u32( list );
            for( std::uint32_t k = 0; k < n; ++k ) {
                text += type( r.u16( list + 4 + 2 * k ) );
            }
        }
        return text + ")" + type( r.u32( at + 4 ) );
    };
    const auto method = [&]( std::uint32_t i ) {
        const std::size_t at = methods_off + 8 * i;
        return type( r.u16( at ) ) + "->" + str( r.u32( at + 4 ) ) + proto( r.u16( at + 2 ) );
    };

    std::printf( "strings\n" );
    for( std::uint32_t i = 0; i < strings; ++i ) {
        std::printf( "  %u  \"%s\"\n", i, str( i ).c_str() );
    }
    std::printf( "methods\n" );
    for( std::uint32_t i = 0; i < methods; ++i ) {
        std::printf( "  %u  %s\n", i, method( i ).c_str() );
    }
    for( std::uint32_t c = 0; c < classes; ++c ) {
        const std::size_t def = classes_off + 32 * c;
        std::printf( "class %s extends %s  [%s]\n", type( r.u32( def ) ).c_str(),
                     type( r.u32( def + 8 ) ).c_str(), flags_text( r.u32( def + 4 ) ).c_str() );
        std::size_t at = r.u32( def + 24 );
        if( at == 0 ) {
            continue;
        }
        r.uleb( at );
        r.uleb( at );
        const std::uint32_t direct = r.uleb( at );
        const std::uint32_t virtuals = r.uleb( at );
        for( std::uint32_t list = 0; list < 2; ++list ) {
            std::uint32_t index = 0;
            for( std::uint32_t m = 0; m < ( list == 0 ? direct : virtuals ); ++m ) {
                index += r.uleb( at );
                const std::uint32_t flags = r.uleb( at );
                const std::uint32_t code = r.uleb( at );
                std::printf( "  %s method %s  [%s]\n", list == 0 ? "direct" : "virtual",
                             method( index ).c_str(), flags_text( flags ).c_str() );
                if( code == 0 ) {
                    std::printf( "    no code%s\n", ( flags & 0x100U ) != 0 ? ": native" : "" );
                    continue;
                }
                std::printf( "    registers %u, ins %u, outs %u, tries %u\n", r.u16( code ),
                             r.u16( code + 2 ), r.u16( code + 4 ), r.u16( code + 6 ) );
                const std::uint32_t units = r.u32( code + 12 );
                for( std::uint32_t pc = 0; pc < units && r.ok; ) {
                    const std::size_t insn = code + 16 + 2 * pc;
                    const std::uint32_t op = r.u8( insn );
                    if( op == 0x0E ) {
                        std::printf( "    %04x  return-void\n", pc );
                        pc += 1;
                    } else if( op == 0x70 || op == 0x71 ) {
                        const std::uint32_t count = r.u8( insn + 1 ) >> 4U;
                        const std::uint32_t regs = r.u16( insn + 4 );
                        std::string args;
                        for( std::uint32_t k = 0; k < count && k < 4; ++k ) {
                            args += ( k == 0 ? "v" : ", v" ) +
                                    std::to_string( ( regs >> ( 4 * k ) ) & 0xFU );
                        }
                        std::printf( "    %04x  %s {%s}, %s\n", pc,
                                     op == 0x70 ? "invoke-direct" : "invoke-static", args.c_str(),
                                     method( r.u16( insn + 2 ) ).c_str() );
                        pc += 3;
                    } else {
                        std::printf( "    %04x  op 0x%02x (not one dexgen writes)\n", pc, op );
                        pc += 1;
                    }
                }
            }
        }
    }
    if( !r.ok ) {
        std::printf( "a table points outside the file\n" );
        good = false;
    }
    return good ? 0 : 1;
}

// A FILE that closes itself.
struct CloseFile
{
        void operator()( std::FILE *file ) const
        {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): this deleter is the FILE's owner
            std::fclose( file );
        }
};
using File = std::unique_ptr<std::FILE, CloseFile>;

bool read_file( const char *path, std::vector<std::uint8_t> &out )
{
    const File file( std::fopen( path, "rb" ) );
    if( file == nullptr ) {
        return false;
    }
    std::uint8_t buffer[4096];
    std::size_t n = 0;
    while( ( n = std::fread( buffer, 1, sizeof( buffer ), file.get() ) ) > 0 ) {
        out.insert( out.end(), buffer, buffer + n );
    }
    return true;
}

} // namespace

int main( int argc, char **argv )
{
    if( argc == 3 && std::string_view( argv[1] ) == "--dump" ) {
        std::vector<std::uint8_t> file;
        if( !read_file( argv[2], file ) ) {
            std::fprintf( stderr, "dexgen: cannot read %s\n", argv[2] );
            return 2;
        }
        return dump( file );
    }
    if( argc != 2 || argv[1][0] == '-' ) {
        std::fprintf( stderr, "usage: dexgen <out.dex> | dexgen --dump <in.dex>\n" );
        return 2;
    }
    const std::vector<std::uint8_t> dex = write_dex( shims() );
    {
        const File file( std::fopen( argv[1], "wb" ) );
        if( file == nullptr ||
            std::fwrite( dex.data(), 1, dex.size(), file.get() ) != dex.size() ) {
            std::fprintf( stderr, "dexgen: cannot write %s\n", argv[1] );
            return 1;
        }
    }
    std::printf( "dexgen: wrote %s, %zu bytes, %zu class(es)\n", argv[1], dex.size(),
                 shims().size() );
    return 0;
}
