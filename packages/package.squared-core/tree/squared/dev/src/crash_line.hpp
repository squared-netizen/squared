#pragma once

#include <cstddef>
#include <cstdint>

namespace sq::dev
{

/**
 * @brief A line of a crash report under construction: plain character
 * appends into a fixed buffer, so a signal handler can use it. No allocation,
 * no stdio, no locale. Text past `capacity` is dropped.
 */
struct CrashLine final
{
        static constexpr std::size_t capacity = 1000;

        char text[capacity + 1]{};
        std::size_t length = 0;

        void add( const char *s ) noexcept
        {
            while( *s != '\0' && length < capacity ) {
                text[length++] = *s++;
            }
        }
        void add_decimal( std::uint64_t v ) noexcept
        {
            char digits[20];
            std::size_t n = 0;
            do {
                digits[n++] = static_cast<char>( '0' + v % 10 );
                v /= 10;
            } while( v != 0 );
            while( n != 0 && length < capacity ) {
                text[length++] = digits[--n];
            }
        }
        void add_hex( std::uint64_t v ) noexcept
        {
            add( "0x" );
            char digits[16];
            std::size_t n = 0;
            do {
                digits[n++] = "0123456789abcdef"[v & 0xF];
                v >>= 4;
            } while( v != 0 );
            while( n != 0 && length < capacity ) {
                text[length++] = digits[--n];
            }
        }
};

} // namespace sq::dev
