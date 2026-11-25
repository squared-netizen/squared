// SPDX-License-Identifier: MIT
//
// sq.cart:zstd — decoding one run of codec 1 or 2 (§4.3) with zstd's
// single-file decoder, trusting nothing the frame says: it is exactly one
// frame and nothing else, its header gives a content size equal to the
// record's, its window is within this reader's limit, and its dictionary
// is the one the record names. A frame header can lie, and an untrusted
// size must never drive an allocation (red-team R5).

module;

#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>

export module sq.cart:zstd;

import std;
import :error;

namespace sq::cart {

struct FrameInfo {
    std::uint64_t content_size = 0;
    std::uint64_t window_size = 0;
    std::uint32_t dictionary_id = 0;
};

/// Check a run before decoding it: one zstd frame filling the run exactly,
/// not a skippable frame, its content size given and equal to `size`, its
/// window within 2^max_window_log, its dictionary ID equal to `dictionary_id`
/// (0: none).
[[nodiscard]] Result<FrameInfo> check_frame(std::span<const std::byte> run, std::uint64_t size,
                                            std::uint32_t dictionary_id, std::uint32_t max_window_log,
                                            const std::string& what) {
    ZSTD_frameHeader h{};
    const std::size_t r = ZSTD_getFrameHeader(&h, run.data(), run.size());
    if (r != 0) return fail(Code::malformed, "not a zstd frame, or a truncated one", what);
    if (h.frameType != ZSTD_frame) return fail(Code::malformed, "a skippable zstd frame where content belongs", what);
    if (h.frameContentSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        return fail(Code::malformed, "a zstd frame that does not give its content size", what);
    }
    if (h.frameContentSize != size) {
        return fail(Code::malformed, std::format("a zstd frame of {} bytes for an entry of {}", h.frameContentSize, size), what);
    }
    const std::size_t frame = ZSTD_findFrameCompressedSize(run.data(), run.size());
    if (ZSTD_isError(frame)) return fail(Code::malformed, std::format("a broken zstd frame: {}", ZSTD_getErrorName(frame)), what);
    if (frame != run.size()) return fail(Code::malformed, "bytes after the zstd frame in its run", what);
    if (h.windowSize > (std::uint64_t{1} << max_window_log)) {
        return fail(Code::unsupported, std::format("a zstd window of {} bytes, past this reader's 2^{}", h.windowSize, max_window_log), what);
    }
    if (h.dictID != dictionary_id) {
        return fail(Code::malformed,
                    dictionary_id == 0 ? std::format("a zstd frame that wants dictionary {}", h.dictID)
                                       : std::format("a zstd frame for dictionary {}, where the record names {}", h.dictID, dictionary_id),
                    what);
    }
    return FrameInfo{h.frameContentSize, h.windowSize, h.dictID};
}

/// Decode a checked run into exactly `size` bytes.
[[nodiscard]] Result<std::vector<std::byte>> zstd_decode(std::span<const std::byte> run, std::uint64_t size,
                                                         std::span<const std::byte> dictionary,
                                                         std::uint32_t max_window_log, const std::string& what) {
    std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)> ctx(ZSTD_createDCtx(), ZSTD_freeDCtx);
    if (!ctx) return fail(Code::io, "out of memory for a zstd decoder", what);
    ZSTD_DCtx_setParameter(ctx.get(), ZSTD_d_windowLogMax, static_cast<int>(max_window_log));
    if (!dictionary.empty()) {
        const std::size_t r = ZSTD_DCtx_loadDictionary(ctx.get(), dictionary.data(), dictionary.size());
        if (ZSTD_isError(r)) return fail(Code::malformed, std::format("an unusable zstd dictionary: {}", ZSTD_getErrorName(r)), what);
    }
    std::vector<std::byte> out(static_cast<std::size_t>(size));
    // The output buffer is exactly the declared size: a frame that would
    // decode to more fails here, and one that decodes to less is caught below.
    const std::size_t got = ZSTD_decompressDCtx(ctx.get(), out.data(), out.size(), run.data(), run.size());
    if (ZSTD_isError(got)) return fail(Code::malformed, std::format("zstd: {}", ZSTD_getErrorName(got)), what);
    if (got != size) return fail(Code::malformed, std::format("a zstd frame that decoded to {} bytes, not {}", got, size), what);
    return out;
}

/// A dictionary's own ID, from its header (0 for a raw-content dictionary).
[[nodiscard]] std::uint32_t dictionary_id_of(std::span<const std::byte> dictionary) {
    return ZSTD_getDictID_fromDict(dictionary.data(), dictionary.size());
}

}  // namespace sq::cart
