#pragma once

#include <cstddef>
#include <string_view>

// The bridge's wire format, shared by the app's end (bridge.cpp) and Termux's
// (client/sqconsole.cpp), so the two cannot drift apart. Text lines, each
// ended by '\n':
//
//   client -> app   "squared-bridge 1 <secret>[ quiet]"   first, once
//                   "<command line>"                     then, each a command
//   app -> client   "ok"                                 the secret matched
//                   "i <text>" "w <text>" "e <text>"     a line the Console printed
//                   "."                                  a command has finished
//
// "quiet" asks for no replay of the scrollback on connect (a scripted client).
// Version 1. A wrong secret gets no answer: the connection is closed.

namespace sq::console::bridge_protocol
{

inline constexpr std::string_view hello = "squared-bridge 1 ";
inline constexpr std::string_view quiet = " quiet";
inline constexpr std::string_view ok = "ok";
inline constexpr std::string_view announce = "squared-bridge up";
inline constexpr char done = '.';
inline constexpr char info = 'i';
inline constexpr char warning = 'w';
inline constexpr char error = 'e';

/// The secret's length, in characters (letters and digits).
inline constexpr std::size_t secret_min = 16;
inline constexpr std::size_t secret_max = 64;

} // namespace sq::console::bridge_protocol
