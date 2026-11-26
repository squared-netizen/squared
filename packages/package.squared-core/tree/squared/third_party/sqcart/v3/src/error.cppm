// SPDX-License-Identifier: MIT
//
// sq.cart:error — what can go wrong, as a value. Every fallible operation
// returns Result<T>, std::expected with an Error; nothing throws past the
// module's boundary.

export module sq.cart:error;

import std;

export namespace sq::cart {

enum class Code : std::uint8_t {
    io,           ///< the file system or a read failed
    malformed,    ///< the bytes are not a conforming 3.0 cartridge
    unsupported,  ///< a version, feature, codec or algorithm this reader lacks
    limit,        ///< past a limit of §3.8
    path,         ///< a path breaks §4.1
    json,         ///< JSON that is not I-JSON (§5)
    integrity,    ///< a digest does not match
    not_found,    ///< no such entry
    busy,         ///< another writer holds the cartridge's lock (§10.2)
    conflict,     ///< the cartridge is not the state a caller built on (a Drive's journal, say)
};

[[nodiscard]] constexpr std::string_view to_string(Code code) noexcept {
    switch (code) {
        case Code::io: return "io";
        case Code::malformed: return "malformed";
        case Code::unsupported: return "unsupported";
        case Code::limit: return "limit";
        case Code::path: return "path";
        case Code::json: return "json";
        case Code::integrity: return "integrity";
        case Code::not_found: return "not_found";
        case Code::busy: return "busy";
        case Code::conflict: return "conflict";
    }
    return "unknown";
}

struct Error {
    Code code = Code::malformed;
    std::string message;
    std::optional<std::string> entry;  ///< the entry it concerns, where one does
};

template <class T>
using Result = std::expected<T, Error>;

}  // namespace sq::cart

namespace sq::cart {

/// An error, ready to return from a function returning Result<T>.
[[nodiscard]] inline std::unexpected<Error> fail(Code code, std::string message,
                                                 std::optional<std::string> entry = std::nullopt) {
    return std::unexpected(Error{code, std::move(message), std::move(entry)});
}

}  // namespace sq::cart
