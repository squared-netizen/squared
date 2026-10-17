#pragma once

#include <squared/console/src/level.hpp>
#include <squared/console/src/limits.hpp>
#include <squared/console/src/line.hpp>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string_view>

namespace sq::console
{

/**
 * @brief The console's history: a ring of text lines, each with a level.
 *
 * It owns no memory. It is a view over storage its owner provides (the
 * Console holds `std::array<char, scrollback_bytes>`), so its size is fixed at
 * compile time and nothing is allocated, ever. When a new line does not fit,
 * the oldest lines are dropped until it does.
 *
 * ```cpp
 * std::array<char, sq::console::scrollback_bytes> storage;
 * sq::console::Scrollback lines( storage );
 * lines.push( sq::console::Level::error, "shader did not link" );
 * for( const auto line : lines ) { draw( line.text, line.level ); }
 * ```
 *
 * Not thread-safe: one thread pushes and reads.
 */
class Scrollback final
{
    public:
        /**
         * @brief Use `storage` for the lines.
         * @param storage At least 64 bytes; kept, not copied. Bytes past the
         * last multiple of 4 are not used.
         */
        explicit Scrollback( std::span<char> storage ) noexcept;

        /**
         * @brief Append text, one line per `\n`-separated part.
         *
         * A trailing newline adds no empty line. A line longer than the
         * storage can hold, or than 65535 bytes, is cut to fit. Drops the
         * oldest lines as needed. Never allocates.
         */
        void push( Level level, std::string_view text ) noexcept;

        /** @brief Forget every line; the dropped count is kept. */
        void clear() noexcept;

        /** @brief Lines held now. */
        [[nodiscard]] std::size_t size() const noexcept { return count_; }

        /** @brief True when no line is held. */
        [[nodiscard]] bool empty() const noexcept { return count_ == 0; }

        /** @brief Lines dropped to make room, since construction. */
        [[nodiscard]] std::uint64_t dropped() const noexcept { return dropped_; }

        /** @brief The longest line push() keeps whole, in bytes. */
        [[nodiscard]] std::size_t longest_line() const noexcept;

        /** @brief Walks the lines, oldest first. */
        class Iterator final
        {
            public:
                using iterator_category = std::forward_iterator_tag;
                using value_type = Line;
                using difference_type = std::ptrdiff_t;
                using pointer = void;
                using reference = Line;

                Iterator() noexcept = default;
                [[nodiscard]] Line operator*() const noexcept;
                Iterator &operator++() noexcept;
                Iterator operator++( int ) noexcept
                {
                    Iterator old = *this;
                    ++*this;
                    return old;
                }
                friend bool operator==( const Iterator &a, const Iterator &b ) noexcept
                {
                    return a.remaining_ == b.remaining_;
                }

            private:
                friend class Scrollback;
                Iterator( const Scrollback *owner, std::uint32_t at,
                          std::size_t remaining ) noexcept
                    : owner_( owner )
                    , at_( at )
                    , remaining_( remaining )
                {}

                const Scrollback *owner_{ nullptr };
                std::uint32_t at_{ 0 };
                std::size_t remaining_{ 0 };
        };

        /** @brief The oldest line. */
        [[nodiscard]] Iterator begin() const noexcept;

        /** @brief Past the newest line. */
        [[nodiscard]] Iterator end() const noexcept { return {}; }

        /**
         * @brief The newest `count` lines, oldest of them first.
         * @return An iterator to walk to end(); all lines if `count` is larger.
         */
        [[nodiscard]] Iterator last( std::size_t count ) const noexcept;

    private:
        void push_line( Level level, std::string_view text ) noexcept;
        void drop_oldest() noexcept;
        [[nodiscard]] std::uint32_t record_size( std::uint32_t at ) const noexcept;
        [[nodiscard]] std::uint32_t next( std::uint32_t at ) const noexcept;

        std::span<char> storage_;
        std::size_t count_{ 0 };
        std::uint64_t dropped_{ 0 };
        std::uint32_t capacity_{ 0 }; // usable bytes: storage size rounded down to 4
        std::uint32_t head_{ 0 };     // the oldest record
        std::uint32_t tail_{ 0 };     // where the next record goes
        std::uint32_t end_{ 0 };      // while wrapped: the end of the upper records
        bool wrapped_{ false };       // records run head_..end_, then 0..tail_
};

static_assert( sizeof( Scrollback ) <= 64, "Scrollback is a view; see docs/developer/console.md" );

} // namespace sq::console
