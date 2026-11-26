module;

#include <zstd.h>

module sq.data;

import std;

namespace sq::data
{

namespace
{

struct DecompressFree
{
        void operator()( ZSTD_DCtx *context ) const noexcept { ZSTD_freeDCtx( context ); }
};

ZstdResult failed( std::string why )
{
    ZstdResult r;
    r.error = std::move( why );
    return r;
}

} // namespace

ZstdResult compress( std::span<const std::byte> bytes, int level ) noexcept
{
    level = std::clamp( level, ZSTD_minCLevel(), ZSTD_maxCLevel() );
    ZstdResult r;
    r.bytes.resize( ZSTD_compressBound( bytes.size() ) );
    const std::size_t made =
        ZSTD_compress( r.bytes.data(), r.bytes.size(), bytes.data(), bytes.size(), level );
    if( ZSTD_isError( made ) != 0U ) {
        return failed( std::string( "zstd: " ) + ZSTD_getErrorName( made ) );
    }
    r.bytes.resize( made );
    return r;
}

ZstdResult decompress( std::span<const std::byte> frames, std::size_t limit ) noexcept
{
    const std::unique_ptr<ZSTD_DCtx, DecompressFree> context( ZSTD_createDCtx() );
    if( context == nullptr ) {
        return failed( "zstd: no memory for a decoder" );
    }
    constexpr std::size_t first = std::size_t{ 64 } << 10U;
    ZstdResult r;
    ZSTD_inBuffer in{ frames.data(), frames.size(), 0 };
    std::size_t used = 0;
    std::size_t hint = 1; // what zstd still wants: 0 at the end of a frame
    for( ;; ) {
        if( used == r.bytes.size() ) {
            if( in.pos == in.size && hint == 0 ) {
                break; // filled exactly, at the end of the last frame
            }
            if( used >= limit ) {
                return failed( std::format( "zstd: more than {} bytes", limit ) );
            }
            r.bytes.resize( std::min( limit, std::max( used * 2, first ) ) );
        }
        ZSTD_outBuffer out{ r.bytes.data() + used, r.bytes.size() - used, 0 };
        const std::size_t before = in.pos;
        hint = ZSTD_decompressStream( context.get(), &out, &in );
        if( ZSTD_isError( hint ) != 0U ) {
            return failed( std::string( "zstd: " ) + ZSTD_getErrorName( hint ) );
        }
        used += out.pos;
        const bool full = out.pos == out.size;
        if( in.pos == in.size && !full ) {
            if( hint != 0 ) {
                return failed( "zstd: the frame ends early" );
            }
            break;
        }
        if( !full && out.pos == 0 && in.pos == before ) {
            return failed( "zstd: no progress" );
        }
    }
    r.bytes.resize( used );
    return r;
}

} // namespace sq::data
