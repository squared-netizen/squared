// SPDX-License-Identifier: MIT
//
// sq.cart:ed25519 — Ed25519 signatures (RFC 8032) through Monocypher's
// optional Ed25519 functions (Appendix B.1: its default EdDSA is BLAKE2b,
// not Ed25519). §9.3 asks for RFC 8032 §5.1.7 verification that refuses a
// non-canonical S or point encoding: Monocypher refuses S >= L itself, but
// accepts a public key or R whose y is not reduced, so those are refused
// here first.

module;

#include <monocypher-ed25519.h>

export module sq.cart:ed25519;

import std;

namespace sq::cart {

/// Whether a 32-byte point encoding is canonical: its y coordinate (the
/// low 255 bits, little-endian) is below p = 2^255 - 19.
[[nodiscard]] bool canonical_point(std::span<const std::byte, 32> encoding) noexcept {
    // y >= p only when the top 250 bits are all ones and the low byte is at
    // least 0xed (p's low byte): 0xed..0xff with 30 bytes of 0xff and a top
    // byte of 0x7f (sign bit masked).
    const auto byte = [&](std::size_t i) { return std::to_integer<unsigned>(encoding[i]); };
    if ((byte(31) & 0x7f) != 0x7f) return true;
    for (std::size_t i = 1; i < 31; ++i) {
        if (byte(i) != 0xff) return true;
    }
    return byte(0) < 0xed;
}

}  // namespace sq::cart

export namespace sq::cart {

/// Verify an Ed25519 signature, refusing non-canonical encodings.
[[nodiscard]] bool ed25519_verify(std::span<const std::byte, 64> signature, std::span<const std::byte, 32> public_key,
                                  std::span<const std::byte> message) noexcept {
    if (!canonical_point(public_key) || !canonical_point(signature.first<32>())) return false;
    return crypto_ed25519_check(reinterpret_cast<const std::uint8_t*>(signature.data()),
                                reinterpret_cast<const std::uint8_t*>(public_key.data()),
                                reinterpret_cast<const std::uint8_t*>(message.data()), message.size()) == 0;
}

/// A key pair from a 32-byte seed: the 64-byte secret key (seed and public
/// key, as Monocypher keeps it) and the public key. The seed is wiped.
inline void ed25519_key_pair(std::array<std::byte, 32>& seed, std::array<std::byte, 64>& secret_key,
                             std::array<std::byte, 32>& public_key) noexcept {
    crypto_ed25519_key_pair(reinterpret_cast<std::uint8_t*>(secret_key.data()),
                            reinterpret_cast<std::uint8_t*>(public_key.data()),
                            reinterpret_cast<std::uint8_t*>(seed.data()));
}

[[nodiscard]] std::array<std::byte, 64> ed25519_sign(const std::array<std::byte, 64>& secret_key,
                                                     std::span<const std::byte> message) noexcept {
    std::array<std::byte, 64> signature{};
    crypto_ed25519_sign(reinterpret_cast<std::uint8_t*>(signature.data()),
                        reinterpret_cast<const std::uint8_t*>(secret_key.data()),
                        reinterpret_cast<const std::uint8_t*>(message.data()), message.size());
    return signature;
}

}  // namespace sq::cart
