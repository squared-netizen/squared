#pragma once

#include <squared/messaging/address.hpp>
#include <squared/messaging/name.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace sq::messaging
{

/**
 * @brief The text of every registered name, for writing telegrams as text.
 *
 * Telegrams carry only a name's id; the dev log, replay files and saves
 * write the text. The table lives in storage its owner gives it (the
 * Telegraph's, sized by `TELEGRAPH_NAMES`), and allocates nothing: 16 bytes
 * per name plus its text, so the default 8 KB holds about 250 names.
 *
 * Registration catches the one thing a 64-bit id can get wrong: two names
 * with one id. Not thread-safe; register before the threads start.
 */
class NameTable final
{
    public:
        /** @brief What `add` did. */
        enum class AddResult : std::uint8_t {
            added,     ///< new
            known,     ///< already registered, same text
            invalid,   ///< not a name (`valid_name`)
            collision, ///< another name has this id: rename one of them
            full,      ///< no room left in the storage
        };

        /**
         * @brief A table over `storage`, empty. The storage must outlive it,
         * and is used as it is: no alignment needed.
         */
        explicit NameTable( std::span<std::byte> storage ) noexcept;

        /** @brief Register a name. */
        [[nodiscard]] AddResult add( std::string_view text ) noexcept;

        /** @brief A registered name's text; empty when it is not registered. */
        [[nodiscard]] std::string_view text( Name name ) const noexcept;

        /**
         * @brief Write `address` as text into `out`: `item[17].name`; a name
         * not registered as `#` and 16 hex digits, then `[uid]`.
         * @return The length written (no terminator), or 0 when `out` is too small.
         */
        [[nodiscard]] std::size_t format( Address address, std::span<char> out ) const noexcept;

        /** @brief How many names are registered. */
        [[nodiscard]] std::size_t size() const noexcept { return count_; }

        /** @brief Bytes in use, entries and text, of `capacity()`. */
        [[nodiscard]] std::size_t used() const noexcept;

        /** @brief The storage's size. */
        [[nodiscard]] std::size_t capacity() const noexcept { return storage_.size(); }

    private:
        /// Kept sorted by id at the front of the storage; the text grows from its end.
        struct Entry
        {
                std::uint64_t id;
                std::uint32_t offset;
                std::uint16_t length;
                std::uint16_t spare;
        };
        static_assert( sizeof( Entry ) == 16 );

        friend struct NameTableProbe; // the test reaches insert(), to make a collision

        [[nodiscard]] AddResult insert( std::uint64_t id, std::string_view text ) noexcept;
        [[nodiscard]] Entry entry( std::size_t index ) const noexcept;
        [[nodiscard]] std::size_t lower_bound( std::uint64_t id ) const noexcept;

        std::span<std::byte> storage_;
        std::uint32_t count_ = 0;
        std::uint32_t text_used_ = 0;
};

} // namespace sq::messaging
