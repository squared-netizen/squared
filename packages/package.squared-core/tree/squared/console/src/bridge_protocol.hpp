#pragma once

#include <cstddef>
#include <string_view>

// The bridge's wire format, shared by the app's end (bridge.cpp) and Termux's
// (client/sqconsole.cpp), so the two cannot drift apart. Text lines, each
// ended by '\n', and file bytes framed by them:
//
//   client -> app   "squared-bridge 1 <secret>[ quiet]"   first, once
//                   "<command line>"                     then, each a command
//                   "push <size> <name>" + size bytes    a file for the app
//   app -> client   "ok"                                 the secret matched
//                   "i <text>" "w <text>" "e <text>"     a line the Console printed
//                   "p <size> <name>"                    a file follows (pull); size "?"
//                                                        when unknown (a /proc file)...
//                   "d <n>" + n bytes                    ...in chunks of at most 4 KB
//                   "."                                  a command has finished; after
//                                                        a pull, the file is complete
//
// "quiet" asks for no replay of the scrollback on connect (a scripted client).
// Lines the Console prints may arrive between a file's chunks. A file's name
// is a plain name, no '/'. Version 1. A wrong secret gets no answer: the
// connection is closed.

namespace sq::console::bridge_protocol
{

inline constexpr std::string_view hello = "squared-bridge 1 ";
inline constexpr std::string_view quiet = " quiet";
inline constexpr std::string_view ok = "ok";
inline constexpr std::string_view announce = "squared-bridge up";
inline constexpr std::string_view push = "push ";
inline constexpr char done = '.';
inline constexpr char info = 'i';
inline constexpr char warning = 'w';
inline constexpr char error = 'e';
inline constexpr char file = 'p';
inline constexpr char data = 'd';

/// The secret's length, in characters (letters and digits).
inline constexpr std::size_t secret_min = 16;
inline constexpr std::size_t secret_max = 64;

/// The most file bytes one "d" frame carries.
inline constexpr std::size_t chunk_bytes = 4096;

/// The longest file name either end accepts.
inline constexpr std::size_t name_max = 96;

/// A name a file may be sent under: 1 to name_max bytes, not "." or "..",
/// no '/', no control characters. Both ends check it.
[[nodiscard]] constexpr bool good_name( std::string_view name ) noexcept
{
    if( name.empty() || name.size() > name_max || name == "." || name == ".." ) {
        return false;
    }
    for( const char c : name ) {
        if( c == '/' || static_cast<unsigned char>( c ) < 0x20 || c == 0x7f ) {
            return false;
        }
    }
    return true;
}

} // namespace sq::console::bridge_protocol
