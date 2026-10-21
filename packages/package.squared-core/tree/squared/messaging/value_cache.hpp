#pragma once

#include <squared/messaging/address.hpp>
#include <squared/messaging/telegram.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace sq::messaging
{

/** @brief What `ValueCache::store` did. */
enum class CacheResult : std::uint8_t {
    /** The address now has this value. */
    stored,
    /** No room: every entry is bound, or the text area is full of bound text. Counted. */
    dropped,
    /** The text or uid list is larger than a quarter of the text area. Counted. */
    too_large,
};

/** @brief A cached value: the telegram that set it, and its text or uid list. */
struct CachedValue
{
        /** For `text` and `uids`, its `DataRef` is `{ 0, data.size() }`: where it is in `data`. */
        Telegram telegram;
        /** The text or uid list; empty for any other type. Valid until the next `store` or `bind`. */
        std::span<const std::byte> data;
};

/** @brief A bound entry found by `ValueCache::bound_from`: its number and its value. */
struct BoundEntry
{
        std::uint32_t entry = 0;
        Address address;
        /** Its value; nothing when it was bound before any value came. */
        std::optional<CachedValue> value;
};

/** @brief How full a cache is and has been (the Console's `telegraph` command). */
struct CacheStats
{
        std::uint32_t entries = 0;
        std::uint32_t used = 0;
        std::uint32_t bound = 0;
        std::uint32_t text_bytes = 0;
        std::uint32_t text_used = 0;
        std::uint32_t highest_text = 0;
        /** Values pushed out to make room for others. */
        std::uint32_t evicted = 0;
        /** Values not kept: `dropped` and `too_large`. */
        std::uint32_t dropped = 0;
};

/**
 * @brief The latest value of each address, on the UI side: a screen built
 * later shows current values at once.
 *
 * An address an actor is bound to (`bind`) keeps its entry; the others are
 * evicted when room is needed, roughly oldest first. Text and uid lists up
 * to 16 bytes are held in the entry; longer ones in a text area, which is
 * compacted when it fills. Over storage its owner gives it, sized at init;
 * nothing allocates. One thread only.
 */
class ValueCache final
{
    public:
        /** @brief Index bytes a cache of `entries` entries needs: 4 an entry. */
        [[nodiscard]] static constexpr std::size_t index_bytes( std::size_t entries ) noexcept
        {
            return entries * 4;
        }

        /**
         * @brief A cache over `entries` (32 bytes an entry, a power of two of
         * them, 2 to 32,768), `index` (`index_bytes`) and `text` (text and
         * uid lists over 16 bytes; may be empty). Asserted; the storage needs
         * no alignment.
         */
        ValueCache( std::span<std::byte> entries, std::span<std::byte> index,
                    std::span<std::byte> text ) noexcept;

        ValueCache( const ValueCache & ) = delete;
        ValueCache &operator=( const ValueCache & ) = delete;
        ValueCache( ValueCache && ) = delete;
        ValueCache &operator=( ValueCache && ) = delete;
        ~ValueCache() = default;

        /**
         * @brief Make `telegram`'s value the latest for its address. A `text`
         * or `uids` value is taken from `data` (as a ring delivers it); any
         * other type takes no data. On `dropped` or `too_large` the address
         * keeps the value it had.
         */
        [[nodiscard]] CacheResult store( const Telegram &telegram,
                                         std::span<const std::byte> data = {} ) noexcept;

        /** @brief The latest value of `address`; nothing if none is cached. */
        [[nodiscard]] std::optional<CachedValue> find( Address address ) const noexcept;

        /**
         * @brief An actor shows `address`: its entry is kept (made now if
         * there is none, with no value yet). False when there is no room (every
         * entry bound) or 255 actors already show it.
         */
        [[nodiscard]] bool bind( Address address ) noexcept;

        /** @brief One actor fewer shows `address`. It must be bound. */
        void unbind( Address address ) noexcept;

        /** @brief How many binds `address` has; 0 when it has none or no entry. */
        [[nodiscard]] std::uint32_t bindings( Address address ) const noexcept;

        /**
         * @brief The first bound entry numbered `from` or after; nothing when
         * there is none. A walk over the bound entries: from 0, then from the
         * last one found plus 1. Unbinding the one found does not disturb it.
         */
        [[nodiscard]] std::optional<BoundEntry> bound_from( std::uint32_t from ) const noexcept;

        [[nodiscard]] CacheStats stats() const noexcept;

    private:
        struct Entry;
        // An entry's number, or none; also a text's offset, or none.
        using MaybeEntry = std::optional<std::uint32_t>;

        [[nodiscard]] Entry load( std::uint32_t at ) const noexcept;
        void save( std::uint32_t at, const Entry &entry ) noexcept;
        [[nodiscard]] std::uint16_t slot_at( std::uint32_t slot ) const noexcept;
        void set_slot( std::uint32_t slot, std::uint16_t entry ) noexcept;
        [[nodiscard]] std::uint32_t home( Name name, std::uint32_t uid ) const noexcept;

        // The slot holding `address`'s entry, or the empty slot where it would go.
        [[nodiscard]] std::uint32_t probe( Address address, bool &found ) const noexcept;
        // An entry for `address`, made if needed: its number, or none.
        [[nodiscard]] MaybeEntry entry_for( Address address ) noexcept;
        // The clock: an unbound entry to reuse (not `keep`), taken out of the index.
        [[nodiscard]] MaybeEntry evict( std::uint32_t keep, bool text_only ) noexcept;
        void unindex( std::uint32_t entry ) noexcept;
        // Room for `length` text bytes at the end of the text area, for `entry`.
        [[nodiscard]] MaybeEntry text_room( std::uint32_t entry, std::uint32_t length ) noexcept;
        void compact() noexcept;
        // The value held by entry `at`, which holds one.
        [[nodiscard]] CachedValue value_at( std::uint32_t at ) const noexcept;
        // The room a long text has: its length, or more after an in-place rewrite.
        [[nodiscard]] std::uint32_t room_of( DataRef text ) const noexcept;

        std::span<std::byte> entries_;
        std::span<std::byte> index_;
        std::span<std::byte> text_;
        std::uint32_t capacity_ = 0;
        std::uint32_t slot_mask_ = 0;
        std::uint32_t fresh_ = 0; // entries handed out so far; those after it never used
        std::uint32_t used_ = 0;  // entries in the index
        std::uint32_t bound_ = 0;
        std::uint32_t hand_ = 0;
        std::uint32_t text_used_ = 0;
        std::uint32_t highest_text_ = 0;
        std::uint32_t evicted_ = 0;
        std::uint32_t dropped_ = 0;
};

static_assert( sizeof( ValueCache ) == 88 );

} // namespace sq::messaging
