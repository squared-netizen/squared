#pragma once

#include <squared/messaging/telegram.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace sq::messaging
{

/** @brief What `TelegramRing::push` did. */
enum class RingResult : std::uint8_t {
    /** In the ring; the consumer will see it. */
    sent,
    /** No room now, in the slots or the data ring; nothing was written. Try later. */
    full,
    /** The text or uid list is larger than a quarter of the data ring: it can never be sent. */
    too_large,
};

/** @brief A telegram at the front of a ring, with its text or uid list. */
struct Received
{
        Telegram telegram;
        /** The value's bytes, for `text` and `uids`, valid until `pop()`; otherwise empty. */
        std::span<const std::byte> data;
};

/** @brief How full a ring has been, for sizing it (the Console's `telegraph` command). */
struct RingStats
{
        std::uint32_t slots = 0;
        std::uint32_t data_bytes = 0;
        /** The most telegrams waiting at once, seen as each was pushed. */
        std::uint32_t highest_slots = 0;
        /** The most data bytes held at once, the skipped end of the ring included. */
        std::uint32_t highest_data = 0;
};

/**
 * @brief One direction of the Telegraph: telegrams from one thread to one
 * other, in order, with no lock and no allocation.
 *
 * Exactly one thread pushes and exactly one thread reads. A text or uid list
 * is copied into the ring's data ring by `push`, and the telegram that
 * arrives says where; it stays there until the telegram is popped. Over
 * storage its owner gives it, sized at init.
 *
 * Neither side ever waits: a full ring says so (`RingResult::full`), and an
 * empty one has no `front()`. Waking a consumer that sleeps is its owner's
 * business.
 */
class TelegramRing final
{
    public:
        /**
         * @brief A ring over `slots` (32 bytes a telegram) and `data` (text and
         * uid lists). Both sizes are powers of two: at least 2 telegrams, and
         * the data at least 16 bytes, or empty for a ring that carries no
         * text. Asserted; the storage needs no alignment.
         */
        TelegramRing( std::span<std::byte> slots, std::span<std::byte> data ) noexcept;

        TelegramRing( const TelegramRing & ) = delete;
        TelegramRing &operator=( const TelegramRing & ) = delete;
        TelegramRing( TelegramRing && ) = delete;
        TelegramRing &operator=( TelegramRing && ) = delete;
        ~TelegramRing() = default;

        // ---- the producer's thread ----

        /**
         * @brief Send `telegram`. A `text` or `uids` telegram takes its value
         * from `data` (its own `DataRef` is replaced by where the bytes went);
         * any other type takes no data. A uid list is 4 bytes a uid.
         */
        [[nodiscard]] RingResult push( const Telegram &telegram,
                                       std::span<const std::byte> data = {} ) noexcept;

        // ---- the consumer's thread ----

        /** @brief The oldest telegram not yet popped; nothing when the ring is empty. */
        [[nodiscard]] std::optional<Received> front() const noexcept;

        /** @brief Done with the front telegram and its data. The ring must not be empty. */
        void pop() noexcept;

        // ---- either thread ----

        /** @brief The ring's sizes and the highest fill so far. */
        [[nodiscard]] RingStats stats() const noexcept;

    private:
        [[nodiscard]] Telegram read_slot( std::uint32_t counter ) const noexcept;
        void write_slot( std::uint32_t counter, const Telegram &telegram ) noexcept;
        [[nodiscard]] std::uint32_t slot_capacity() const noexcept { return slot_mask_ + 1; }
        [[nodiscard]] std::uint32_t data_capacity() const noexcept
        {
            return data_.empty() ? 0 : data_mask_ + 1;
        }

        // Read-only after construction: shared by both threads.
        std::span<std::byte> slots_;
        std::span<std::byte> data_;
        std::uint32_t slot_mask_ = 0;
        std::uint32_t data_mask_ = 0;

        // Written by the producer. Free-running counters: an index is the
        // counter masked by the capacity, which divides 2^32, so wrapping is
        // harmless. Apart from the consumer's line, so the two threads do
        // not take one cache line from each other at every telegram.
        struct alignas( 64 ) Producer
        {
                std::atomic<std::uint32_t> head{ 0 };
                std::uint32_t data_head = 0;
                std::atomic<std::uint32_t> highest_slots{ 0 };
                std::atomic<std::uint32_t> highest_data{ 0 };
        };
        // Written by the consumer.
        struct alignas( 64 ) Consumer
        {
                std::atomic<std::uint32_t> tail{ 0 };
                std::atomic<std::uint32_t> data_tail{ 0 };
        };
        Producer producer_;
        Consumer consumer_;
};

static_assert( sizeof( TelegramRing ) == 192 );

} // namespace sq::messaging
