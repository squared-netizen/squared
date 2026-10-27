#pragma once

#include <squared/messaging/address.hpp>
#include <squared/messaging/name.hpp>
#include <squared/messaging/src/data_ref.hpp>
#include <squared/messaging/src/value_type.hpp>
#include <squared/messaging/src/verb.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace sq::messaging
{

/**
 * @brief One message: 32 bytes, copied whole through the Telegraph's rings.
 *
 * | Bytes | Field |
 * |---|---|
 * | 8 | the name's id |
 * | 4 | the uid (0: none) |
 * | 2 | the correlation, which pairs a `do` with the `event` answering it (0: none) |
 * | 2 | the verb (2 bits) and the value type (4 bits); the other 10 bits are 0 |
 * | 16 | the value: a bool, an integer, a double, up to four floats, or where its text or uid list is in the data area |
 *
 * Made by the factories, one per value type, so a telegram is always valid:
 * the bytes a value does not use are zero, which makes two equal telegrams
 * equal byte for byte. Apps send through the Telegraph with typed keys; this
 * is what travels underneath.
 */
class Telegram final
{
    public:
        /** @brief A telegram with no value (`event game.over`). */
        [[nodiscard]] static Telegram empty( Verb verb, Address to ) noexcept;
        [[nodiscard]] static Telegram boolean( Verb verb, Address to, bool value ) noexcept;
        [[nodiscard]] static Telegram integer( Verb verb, Address to, std::int64_t value ) noexcept;
        /** @brief A double; a NaN has no text form, so it is not a value (asserted). */
        [[nodiscard]] static Telegram number( Verb verb, Address to, double value ) noexcept;
        /** @brief Two to four floats (`vec2`..`vec4`); a NaN is not a value (asserted). */
        [[nodiscard]] static Telegram floats( Verb verb, Address to,
                                              std::span<const float> values ) noexcept;
        /** @brief Text or a uid list, by where it is in the data area. */
        [[nodiscard]] static Telegram data( Verb verb, Address to, ValueType type,
                                            DataRef where ) noexcept;

        /** @brief The same telegram, answering or awaiting `correlation`. */
        [[nodiscard]] Telegram with_correlation( std::uint16_t correlation ) const noexcept;

        /** @brief The same telegram, to `uid` of its address: a list row's item. */
        [[nodiscard]] Telegram with_uid( std::uint32_t uid ) const noexcept;

        [[nodiscard]] Verb verb() const noexcept;
        [[nodiscard]] ValueType type() const noexcept;
        [[nodiscard]] Address address() const noexcept { return { name_, uid_ }; }
        [[nodiscard]] std::uint16_t correlation() const noexcept { return correlation_; }

        /** @brief The value, read as its type; meaningless for any other type. */
        [[nodiscard]] bool as_boolean() const noexcept;
        [[nodiscard]] std::int64_t as_integer() const noexcept;
        [[nodiscard]] double as_number() const noexcept;
        /** @brief The floats; `float_count(type())` of them are meaningful. */
        [[nodiscard]] std::array<float, 4> as_floats() const noexcept;
        [[nodiscard]] DataRef as_data() const noexcept;

        /**
         * @brief Whether these 32 bytes are a telegram: a verb and a known
         * type, the spare bits and unused value bytes zero, no NaN. Bytes
         * that arrived from elsewhere (a file) are checked with it; the
         * factories make only valid ones.
         */
        [[nodiscard]] bool valid() const noexcept;

        friend bool operator==( const Telegram &, const Telegram & ) noexcept = default;

    private:
        Telegram() noexcept = default;
        [[nodiscard]] static Telegram make( Verb verb, Address to, ValueType type ) noexcept;

        Name name_;
        std::uint32_t uid_ = 0;
        std::uint16_t correlation_ = 0;
        std::uint16_t kind_ = 0;
        std::array<std::byte, 16> value_{};

        friend struct TelegramBytes; // the tests and file readers see raw bytes
};

static_assert( sizeof( Telegram ) == 32 );
static_assert( std::is_trivially_copyable_v<Telegram> );

/**
 * @brief A telegram's 32 bytes, both ways, for reading telegrams that were
 * stored (`valid()` then says whether they are one).
 */
struct TelegramBytes
{
        [[nodiscard]] static std::array<std::byte, 32> of( const Telegram &telegram ) noexcept;
        [[nodiscard]] static Telegram from( const std::array<std::byte, 32> &bytes ) noexcept;
};

} // namespace sq::messaging
