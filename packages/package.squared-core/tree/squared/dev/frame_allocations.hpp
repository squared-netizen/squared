#pragma once

#include <cstdint>

namespace sq::dev
{

/// Heap allocations made by one thread between frame_begin() and frame_end().
struct FrameAllocations
{
        std::uint64_t count = 0; ///< Calls to operator new, every form.
        std::uint64_t bytes = 0; ///< Bytes those calls asked for.
};

/**
 * @brief Start counting heap allocations made by the calling thread.
 *
 * Linking this module replaces the global operator new and operator delete
 * with versions that count, then use malloc and free. Only allocations on the
 * thread that called frame_begin(), and only until frame_end(), are counted;
 * a loader or audio thread allocating meanwhile is not. Calling it twice
 * without frame_end() restarts the count.
 */
void frame_begin() noexcept;

/**
 * @brief Stop counting and report what the frame allocated.
 *
 * When a log sink is open (open_log_sink()), allocating frames are reported
 * through it: the first one at once, then at most one summary line every 120
 * frames, followed by the window's busiest call sites. Each call site is named
 * once, the first time it allocates, as the four innermost frames above
 * operator new, in the `library 0xoffset` form addr2line takes. A frame that
 * allocated nothing is never reported.
 *
 * @return This frame's count; zero when frame_begin() was not called on this
 * thread.
 */
FrameAllocations frame_end() noexcept;

} // namespace sq::dev
