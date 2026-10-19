#pragma once

#include <unistd.h>

#include <cstddef>
#include <cstring>
#include <span>
#include <string_view>

namespace sq::console
{

/**
 * @brief Read `fd` to its end through `buffer`, calling `line( text )` for each
 * line, without its newline; the last line counts even with no newline after
 * it. A line longer than the buffer arrives in buffer-sized pieces. Nothing is
 * allocated: the buffer is the caller's, usually a few KB on the stack.
 */
template<typename Line>
void for_each_line( int fd, std::span<char> buffer, Line line )
{
    std::size_t held = 0;
    bool reading = true;
    while( reading ) {
        const ssize_t got = ::read( fd, buffer.data() + held, buffer.size() - held );
        reading = got > 0;
        held += reading ? static_cast<std::size_t>( got ) : 0;
        std::size_t start = 0;
        for( ;; ) {
            const auto end = std::string_view( buffer.data() + start, held - start ).find( '\n' );
            if( end == std::string_view::npos ) {
                break;
            }
            line( std::string_view( buffer.data() + start, end ) );
            start += end + 1;
        }
        if( !reading || ( start == 0 && held == buffer.size() ) ) {
            // The end of the file, or a line that fills the buffer: hand on what is held.
            if( held > start ) {
                line( std::string_view( buffer.data() + start, held - start ) );
            }
            held = 0;
        } else {
            std::memmove( buffer.data(), buffer.data() + start, held - start );
            held -= start;
        }
    }
}

} // namespace sq::console
