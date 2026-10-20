#pragma once

#include <squared/messaging/src/data_ref.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace sq::messaging
{

/**
 * @brief A data area for telegrams' text and uid lists, filled from the
 * front: where `read_telegram` puts what it reads, and what a test or a
 * replay file hands `write_telegram`. Over storage its owner gives it; it
 * allocates nothing. (The Telegraph's rings keep their own.)
 */
class TelegramData final
{
    public:
        explicit TelegramData( std::span<std::byte> storage ) noexcept
            : storage_( storage )
        {}

        /** @brief Copy `bytes` in. Nothing when they do not fit. */
        [[nodiscard]] std::optional<DataRef> append( std::span<const std::byte> bytes ) noexcept;

        /** @brief The bytes at `where`; empty when `where` is outside what was appended. */
        [[nodiscard]] std::span<const std::byte> at( DataRef where ) const noexcept;

        /** @brief Everything appended, for `write_telegram`. */
        [[nodiscard]] std::span<const std::byte> bytes() const noexcept
        {
            return storage_.first( used_ );
        }

        /**
         * @brief Room for `length` bytes at the end, to be filled through
         * `writable`: for a reader that decodes in place. Nothing when it
         * does not fit.
         */
        [[nodiscard]] std::optional<DataRef> reserve( std::size_t length ) noexcept;

        /** @brief The bytes at `where`, to fill; empty outside what was appended. */
        [[nodiscard]] std::span<std::byte> writable( DataRef where ) noexcept;

        /** @brief Forget what was appended after the first `used` bytes. */
        void rewind( std::size_t used ) noexcept { used_ = used < used_ ? used : used_; }

        /** @brief Forget everything appended. */
        void clear() noexcept { used_ = 0; }

        [[nodiscard]] std::size_t used() const noexcept { return used_; }
        [[nodiscard]] std::size_t capacity() const noexcept { return storage_.size(); }

    private:
        std::span<std::byte> storage_;
        std::size_t used_ = 0;
};

} // namespace sq::messaging
