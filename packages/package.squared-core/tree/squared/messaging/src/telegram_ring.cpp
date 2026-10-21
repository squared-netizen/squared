#include <squared/messaging/telegram_ring.hpp>

#include <array>
#include <cassert>
#include <cstring>

namespace sq::messaging
{

namespace
{

constexpr std::size_t slot_size = sizeof( Telegram );

[[nodiscard, maybe_unused]] constexpr bool power_of_two( std::size_t n ) noexcept
{
    return n != 0 && ( n & ( n - 1 ) ) == 0;
}

[[nodiscard]] constexpr bool carries_data( ValueType type ) noexcept
{
    return type == ValueType::text || type == ValueType::uids;
}

// Raise `highest` to `value` if it is lower. Only the producer writes it.
void raise( std::atomic<std::uint32_t> &highest, std::uint32_t value ) noexcept
{
    if( value > highest.load( std::memory_order_relaxed ) ) {
        highest.store( value, std::memory_order_relaxed );
    }
}

} // namespace

TelegramRing::TelegramRing( std::span<std::byte> slots, std::span<std::byte> data ) noexcept
    : slots_( slots )
    , data_( data )
{
    assert( slots.size() % slot_size == 0 );
    assert( power_of_two( slots.size() / slot_size ) && slots.size() / slot_size >= 2 );
    assert( slots.size() / slot_size <= ( std::size_t{ 1 } << 31 ) );
    assert( data.empty() || ( power_of_two( data.size() ) && data.size() >= 16 ) );
    assert( data.size() <= ( std::size_t{ 1 } << 31 ) );
    slot_mask_ = static_cast<std::uint32_t>( slots.size() / slot_size - 1 );
    data_mask_ = data.empty() ? 0 : static_cast<std::uint32_t>( data.size() - 1 );
}

Telegram TelegramRing::read_slot( std::uint32_t counter ) const noexcept
{
    std::array<std::byte, 32> bytes{};
    const std::size_t index = counter & slot_mask_;
    std::memcpy( bytes.data(), slots_.data() + index * slot_size, slot_size );
    return TelegramBytes::from( bytes );
}

void TelegramRing::write_slot( std::uint32_t counter, const Telegram &telegram ) noexcept
{
    const std::array<std::byte, 32> bytes = TelegramBytes::of( telegram );
    const std::size_t index = counter & slot_mask_;
    std::memcpy( slots_.data() + index * slot_size, bytes.data(), slot_size );
}

RingResult TelegramRing::push( const Telegram &telegram, std::span<const std::byte> data ) noexcept
{
    const ValueType type = telegram.type();
    const bool with_data = carries_data( type );
    assert( with_data || data.empty() );
    assert( type != ValueType::uids || data.size() % 4 == 0 );

    // With the skipped end, a value can need almost twice its size, so one
    // up to half the data ring always fits in an empty ring. The limit is a
    // quarter (decided 2026-10-20): a second value of the largest size then
    // fits while one waits, so one big value never holds up the next.
    if( with_data && data.size() > data_capacity() / 4 ) {
        return RingResult::too_large;
    }

    const std::uint32_t head = producer_.head.load( std::memory_order_relaxed );
    const std::uint32_t tail = consumer_.tail.load( std::memory_order_acquire );
    if( head - tail == slot_capacity() ) {
        return RingResult::full;
    }

    Telegram sent = telegram;
    std::uint32_t data_head = producer_.data_head;
    if( with_data ) {
        DataRef where{};
        if( !data.empty() ) {
            const auto length = static_cast<std::uint32_t>( data.size() );
            const std::uint32_t capacity = data_capacity();
            const std::uint32_t index = data_head & data_mask_;
            // A value is never split across the end: when it would not fit
            // before the end, the rest of the ring is skipped and it starts
            // at 0. The consumer finds the skip the same way.
            const std::uint32_t skip = index + length > capacity ? capacity - index : 0;
            const std::uint32_t data_tail = consumer_.data_tail.load( std::memory_order_acquire );
            const std::uint32_t free = capacity - ( data_head - data_tail );
            if( skip + length > free ) {
                return RingResult::full;
            }
            const std::uint32_t start = data_head + skip;
            where = { start & data_mask_, length };
            std::memcpy( data_.data() + where.offset, data.data(), length );
            data_head = start + length;
            raise( producer_.highest_data, data_head - data_tail );
        }
        const Telegram moved = Telegram::data( telegram.verb(), telegram.address(), type, where );
        sent = moved.with_correlation( telegram.correlation() );
    }

    write_slot( head, sent );
    producer_.data_head = data_head;
    // The release publishes the slot and the data bytes together.
    producer_.head.store( head + 1, std::memory_order_release );
    raise( producer_.highest_slots, head + 1 - tail );
    return RingResult::sent;
}

std::optional<Received> TelegramRing::front() const noexcept
{
    const std::uint32_t tail = consumer_.tail.load( std::memory_order_relaxed );
    if( producer_.head.load( std::memory_order_acquire ) == tail ) {
        return std::nullopt;
    }
    Received received{ read_slot( tail ), {} };
    if( carries_data( received.telegram.type() ) ) {
        const DataRef where = received.telegram.as_data();
        assert( where.length <= data_.size() && where.offset <= data_.size() - where.length );
        received.data = std::span<const std::byte>( data_ ).subspan( where.offset, where.length );
    }
    return received;
}

void TelegramRing::pop() noexcept
{
    const std::uint32_t tail = consumer_.tail.load( std::memory_order_relaxed );
    assert( producer_.head.load( std::memory_order_acquire ) != tail );

    const Telegram telegram = read_slot( tail );
    const DataRef where = carries_data( telegram.type() ) ? telegram.as_data() : DataRef{};
    if( where.length != 0 ) {
        // Values are popped in the order they were written, so this one
        // starts where the last one ended, or at 0 if the end was skipped.
        const std::uint32_t data_tail = consumer_.data_tail.load( std::memory_order_relaxed );
        const std::uint32_t index = data_tail & data_mask_;
        const std::uint32_t skip = index == where.offset ? 0 : data_capacity() - index;
        const std::uint32_t start = data_tail + skip;
        assert( ( start & data_mask_ ) == where.offset );
        consumer_.data_tail.store( start + where.length, std::memory_order_release );
    }
    consumer_.tail.store( tail + 1, std::memory_order_release );
}

RingStats TelegramRing::stats() const noexcept
{
    RingStats stats;
    stats.slots = slot_capacity();
    stats.data_bytes = data_capacity();
    stats.highest_slots = producer_.highest_slots.load( std::memory_order_relaxed );
    stats.highest_data = producer_.highest_data.load( std::memory_order_relaxed );
    return stats;
}

} // namespace sq::messaging
