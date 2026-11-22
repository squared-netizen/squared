export module sq.term:src.utf8_encoder;

import std;

export namespace sq::term::detail
{

/**
 * UTF-8 for one character, into `out`; returns how many bytes (1 to 4).
 * U+FFFD for a surrogate or anything past U+10FFFF. The other half of
 * `Utf8Decoder`: what copy_text writes, and what squared-tui's renderer
 * sends.
 */
[[nodiscard]] constexpr std::size_t encode_utf8( char32_t code, std::array<char, 4> &out ) noexcept
{
    if( ( code >= 0xD800 && code <= 0xDFFF ) || code > 0x10FFFF ) {
        code = 0xFFFD;
    }
    const auto byte = []( char32_t value ) { return static_cast<char>( value ); };
    if( code < 0x80 ) {
        out[0] = byte( code );
        return 1;
    }
    if( code < 0x800 ) {
        out[0] = byte( 0xC0 | ( code >> 6 ) );
        out[1] = byte( 0x80 | ( code & 0x3F ) );
        return 2;
    }
    if( code < 0x10000 ) {
        out[0] = byte( 0xE0 | ( code >> 12 ) );
        out[1] = byte( 0x80 | ( ( code >> 6 ) & 0x3F ) );
        out[2] = byte( 0x80 | ( code & 0x3F ) );
        return 3;
    }
    out[0] = byte( 0xF0 | ( code >> 18 ) );
    out[1] = byte( 0x80 | ( ( code >> 12 ) & 0x3F ) );
    out[2] = byte( 0x80 | ( ( code >> 6 ) & 0x3F ) );
    out[3] = byte( 0x80 | ( code & 0x3F ) );
    return 4;
}

/** UTF-8 for `code`, appended to `out`. */
inline void append_utf8( std::string &out, char32_t code )
{
    std::array<char, 4> bytes{};
    out.append( bytes.data(), encode_utf8( code, bytes ) );
}

} // namespace sq::term::detail
