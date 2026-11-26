// SPDX-License-Identifier: MIT
//
// sq.cart:sha256 — SHA-256 (FIPS 180-4), every digest format 3.0 uses
// (Appendix B.1), and the entry digest built from it: a tree over 1 MiB
// leaves (§9.1).

export module sq.cart:sha256;

import std;

export namespace sq::cart {

using Digest = std::array<std::byte, 32>;

/// The size of a chunk, and of a leaf of the entry digest's tree (§4.5,
/// §9.1, Appendix B.1).
inline constexpr std::uint64_t chunk_size = std::uint64_t{1} << 20;

/// Incremental SHA-256.
class Sha256 {
public:
    void update(std::span<const std::byte> bytes) noexcept {
        total_ += bytes.size();
        for (const std::byte b : bytes) {
            block_[held_++] = std::to_integer<std::uint8_t>(b);
            if (held_ == 64) {
                compress();
                held_ = 0;
            }
        }
    }
    void update(std::string_view text) noexcept { update(std::as_bytes(std::span(text))); }

    [[nodiscard]] Digest finish() noexcept {
        const std::uint64_t bits = total_ * 8;
        block_[held_++] = 0x80;
        if (held_ > 56) {
            std::fill(block_.begin() + held_, block_.end(), std::uint8_t{0});
            compress();
            held_ = 0;
        }
        std::fill(block_.begin() + held_, block_.begin() + 56, std::uint8_t{0});
        for (int i = 0; i < 8; ++i) block_[static_cast<std::size_t>(56 + i)] = static_cast<std::uint8_t>(bits >> (56 - 8 * i));
        compress();
        Digest out{};
        for (std::size_t i = 0; i < 8; ++i) {
            for (std::size_t k = 0; k < 4; ++k) out[i * 4 + k] = static_cast<std::byte>(h_[i] >> (24 - 8 * k));
        }
        return out;
    }

private:
    void compress() noexcept {
        static constexpr std::array<std::uint32_t, 64> k{
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16; ++i) {
            w[i] = std::uint32_t{block_[4 * i]} << 24 | std::uint32_t{block_[4 * i + 1]} << 16 |
                   std::uint32_t{block_[4 * i + 2]} << 8 | std::uint32_t{block_[4 * i + 3]};
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const std::uint32_t s0 = std::rotr(w[i - 15], 7) ^ std::rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = std::rotr(w[i - 2], 17) ^ std::rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        auto [a, b, c, d, e, f, g, h] = h_;
        for (std::size_t i = 0; i < 64; ++i) {
            const std::uint32_t t1 = h + (std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
            const std::uint32_t t2 = (std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            h = g, g = f, f = e, e = d + t1, d = c, c = b, b = a, a = t1 + t2;
        }
        h_[0] += a, h_[1] += b, h_[2] += c, h_[3] += d, h_[4] += e, h_[5] += f, h_[6] += g, h_[7] += h;
    }

    std::array<std::uint32_t, 8> h_{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::array<std::uint8_t, 64> block_{};
    std::size_t held_ = 0;
    std::uint64_t total_ = 0;
};

[[nodiscard]] inline Digest sha256(std::span<const std::byte> bytes) noexcept {
    Sha256 h;
    h.update(bytes);
    return h.finish();
}

/// §9.1 from the leaves: SHA-256 of "squared-tree/1", a zero byte, the
/// size as a little-endian u64, and the leaf hashes in order.
[[nodiscard]] inline Digest tree_hash_from_leaves(std::span<const Digest> leaves, std::uint64_t size) noexcept {
    Sha256 h;
    h.update(std::string_view("squared-tree/1\0", 15));
    std::array<std::byte, 8> le{};
    for (std::size_t i = 0; i < 8; ++i) le[i] = static_cast<std::byte>(size >> (8 * i));
    h.update(le);
    for (const Digest& leaf : leaves) h.update(leaf);
    return h.finish();
}

/// The entry digest of §9.1: SHA-256 of content up to 1 MiB (what
/// sha256sum prints), else the tree over 1 MiB leaves.
[[nodiscard]] inline Digest tree_hash(std::span<const std::byte> content) noexcept {
    if (content.size() <= chunk_size) return sha256(content);
    std::vector<Digest> leaves;
    for (std::size_t at = 0; at < content.size(); at += chunk_size) {
        leaves.push_back(sha256(content.subspan(at, std::min<std::size_t>(chunk_size, content.size() - at))));
    }
    return tree_hash_from_leaves(leaves, content.size());
}

/// 64 lowercase hex digits.
[[nodiscard]] inline std::string to_hex(std::span<const std::byte> bytes) {
    std::string out;
    for (const std::byte b : bytes) out += std::format("{:02x}", std::to_integer<unsigned>(b));
    return out;
}

}  // namespace sq::cart
