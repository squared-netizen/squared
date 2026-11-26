// sq.data's zstd: bytes compressed and decompressed, squared's one copy of
// zstd (third_party/zstd, the library squared.zstd) behind two functions.
// What wants more (streaming, dictionaries) includes <zstd.h> itself and
// links the same library.

export module sq.data:zstd;

import std;

export namespace sq::data
{

/** @brief What compress or decompress made, or why it could not. */
struct ZstdResult
{
        /** @brief The bytes made; empty when it failed. */
        std::vector<std::byte> bytes;
        /** @brief Why it failed (zstd's own words); empty when it did not. */
        std::string error;

        [[nodiscard]] explicit operator bool() const noexcept { return error.empty(); }
};

/** @brief zstd's own default: quick, and most of the gain. */
inline constexpr int default_zstd_level = 3;
/** @brief The most decompress makes unless told otherwise: 256 MiB. */
inline constexpr std::size_t default_decompress_limit = std::size_t{ 256 } << 20U;

/**
 * @brief `bytes` as one zstd frame. `level` is clamped to zstd's range
 * (1 to 22; below 1 are its faster levels).
 */
[[nodiscard]] ZstdResult compress( std::span<const std::byte> bytes,
                                   int level = default_zstd_level ) noexcept;

/**
 * @brief One zstd frame or several, one after another, decompressed whole.
 * More than `limit` bytes is an error (a frame need not say its size, and
 * a small one can say a great deal), as are a truncated frame and bytes
 * that are none.
 */
[[nodiscard]] ZstdResult decompress( std::span<const std::byte> frames,
                                     std::size_t limit = default_decompress_limit ) noexcept;

} // namespace sq::data
