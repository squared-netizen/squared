#include <squared/messaging/value_cache.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>

namespace sq::messaging
{

namespace
{

constexpr std::size_t entry_size = 32;
constexpr std::uint16_t empty_slot = 0xFFFF;
constexpr std::uint32_t no_entry = 0xFFFFFFFF;
constexpr std::size_t inline_max = 16;
constexpr std::uint32_t header_bytes = 4; // a long text's entry and room, 2 bytes each

// The flags byte: three bits, then the inline length plus one (0: not inline).
constexpr std::uint8_t used_flag = 0x01;   // in the index
constexpr std::uint8_t recent_flag = 0x02; // written since the clock last passed
constexpr std::uint8_t valued_flag = 0x04; // has a value (bind makes an entry without one)
constexpr unsigned inline_shift = 3;

[[nodiscard, maybe_unused]] constexpr bool power_of_two( std::size_t n ) noexcept
{
    return n != 0 && ( n & ( n - 1 ) ) == 0;
}

[[nodiscard]] constexpr bool carries_data( ValueType type ) noexcept
{
    return type == ValueType::text || type == ValueType::uids;
}

template<typename T>
[[nodiscard]] T read_at( std::span<const std::byte> bytes, std::size_t at ) noexcept
{
    T value{};
    std::memcpy( &value, bytes.data() + at, sizeof( T ) );
    return value;
}

template<typename T>
void write_at( std::span<std::byte> bytes, std::size_t at, T value ) noexcept
{
    std::memcpy( bytes.data() + at, &value, sizeof( T ) );
}

} // namespace

// 32 bytes, laid out as a telegram's, with its correlation's two bytes used
// for the bound count and the flags:
//   0-7 name, 8-11 uid, 12 bound, 13 flags, 14-15 kind, 16-31 value
// The value is the telegram's, or text up to 16 bytes, or where a longer
// text is in the text area (offset and length, 4 bytes each).
struct ValueCache::Entry
{
        std::array<std::byte, entry_size> bytes{};

        [[nodiscard]] Name name() const noexcept
        {
            return Name::from_id( read_at<std::uint64_t>( bytes, 0 ) );
        }
        [[nodiscard]] std::uint32_t uid() const noexcept
        {
            return read_at<std::uint32_t>( bytes, 8 );
        }
        [[nodiscard]] std::uint8_t bound() const noexcept
        {
            return read_at<std::uint8_t>( bytes, 12 );
        }
        [[nodiscard]] std::uint8_t flags() const noexcept
        {
            return read_at<std::uint8_t>( bytes, 13 );
        }
        [[nodiscard]] std::uint16_t kind() const noexcept
        {
            return read_at<std::uint16_t>( bytes, 14 );
        }
        [[nodiscard]] ValueType type() const noexcept
        {
            return static_cast<ValueType>( ( kind() >> 2 ) & 0xF );
        }
        [[nodiscard]] bool has( std::uint8_t flag ) const noexcept
        {
            return ( flags() & flag ) != 0;
        }
        [[nodiscard]] bool is_inline() const noexcept { return ( flags() >> inline_shift ) != 0; }
        [[nodiscard]] std::uint32_t inline_length() const noexcept
        {
            return static_cast<std::uint32_t>( flags() >> inline_shift ) - 1;
        }
        // A value in the text area.
        [[nodiscard]] bool has_text() const noexcept
        {
            return has( valued_flag ) && carries_data( type() ) && !is_inline();
        }
        [[nodiscard]] DataRef text() const noexcept
        {
            return { read_at<std::uint32_t>( bytes, 16 ), read_at<std::uint32_t>( bytes, 20 ) };
        }

        void set_address( Address address ) noexcept
        {
            write_at( bytes, 0, address.name.id );
            write_at( bytes, 8, address.uid );
        }
        void set_bound( std::uint8_t bound ) noexcept { write_at( bytes, 12, bound ); }
        void set_flags( std::uint8_t flags ) noexcept { write_at( bytes, 13, flags ); }
        void set_text( DataRef where ) noexcept
        {
            write_at( bytes, 16, where.offset );
            write_at( bytes, 20, where.length );
        }
};

ValueCache::ValueCache( std::span<std::byte> entries, std::span<std::byte> index,
                        std::span<std::byte> text ) noexcept
    : entries_( entries )
    , index_( index )
    , text_( text )
{
    assert( entries.size() % entry_size == 0 );
    assert( power_of_two( entries.size() / entry_size ) && entries.size() / entry_size >= 2 );
    assert( entries.size() / entry_size <= 32768 );
    assert( index.size() == index_bytes( entries.size() / entry_size ) );
    // A long text's room is 2 bytes, and a text is at most a quarter of the area.
    assert( text.size() <= std::size_t{ 4 } * 0xFFFF );
    capacity_ = static_cast<std::uint32_t>( entries.size() / entry_size );
    slot_mask_ = capacity_ * 2 - 1;
    std::fill( index_.begin(), index_.end(), std::byte{ 0xFF } ); // every slot empty_slot
}

ValueCache::Entry ValueCache::load( std::uint32_t at ) const noexcept
{
    Entry entry;
    std::memcpy( entry.bytes.data(), entries_.data() + std::size_t{ at } * entry_size, entry_size );
    return entry;
}

void ValueCache::save( std::uint32_t at, const Entry &entry ) noexcept
{
    std::memcpy( entries_.data() + std::size_t{ at } * entry_size, entry.bytes.data(), entry_size );
}

std::uint16_t ValueCache::slot_at( std::uint32_t slot ) const noexcept
{
    return read_at<std::uint16_t>( index_, std::size_t{ slot } * 2 );
}

void ValueCache::set_slot( std::uint32_t slot, std::uint16_t entry ) noexcept
{
    write_at( index_, std::size_t{ slot } * 2, entry );
}

std::uint32_t ValueCache::home( Name name, std::uint32_t uid ) const noexcept
{
    // The name is a hash already; the uid is mixed in, then the bits are
    // spread so the low ones (the slot) depend on all of them.
    std::uint64_t h = name.id ^ ( std::uint64_t{ uid } * 0x9E3779B97F4A7C15ULL );
    h ^= h >> 29;
    h *= 0xBF58476D1CE4E5B9ULL;
    h ^= h >> 32;
    return static_cast<std::uint32_t>( h ) & slot_mask_;
}

std::uint32_t ValueCache::probe( Address address, bool &found ) const noexcept
{
    // At most half the slots are taken, so an empty one is always reached.
    std::uint32_t slot = home( address.name, address.uid );
    for( ;; ) {
        const std::uint16_t at = slot_at( slot );
        if( at == empty_slot ) {
            found = false;
            return slot;
        }
        const Entry entry = load( at );
        if( entry.name() == address.name && entry.uid() == address.uid ) {
            found = true;
            return slot;
        }
        slot = ( slot + 1 ) & slot_mask_;
    }
}

ValueCache::MaybeEntry ValueCache::entry_for( Address address ) noexcept
{
    bool found = false;
    std::uint32_t slot = probe( address, found );
    if( found ) {
        return slot_at( slot );
    }
    std::uint32_t at = 0;
    if( fresh_ < capacity_ ) {
        at = fresh_++;
    } else {
        const auto reused = evict( no_entry, false );
        if( !reused ) {
            return std::nullopt;
        }
        at = *reused;
        slot = probe( address, found ); // the eviction may have moved slots
    }
    Entry entry;
    entry.set_address( address );
    entry.set_flags( used_flag | recent_flag );
    save( at, entry );
    set_slot( slot, static_cast<std::uint16_t>( at ) );
    ++used_;
    return at;
}

ValueCache::MaybeEntry ValueCache::evict( std::uint32_t keep, bool text_only ) noexcept
{
    // The clock: the hand goes round the entries. A bound entry is passed;
    // a recently written one loses its mark and is passed; the first other
    // is taken. Two turns see every entry with its mark cleared.
    for( std::uint32_t step = 0; step < 2 * capacity_; ++step ) {
        const std::uint32_t at = hand_;
        hand_ = ( hand_ + 1 ) & ( capacity_ - 1 );
        if( at >= fresh_ || at == keep ) {
            continue;
        }
        Entry entry = load( at );
        if( !entry.has( used_flag ) ) {
            if( text_only ) {
                continue; // holds no text to free
            }
            return at; // free already (its text was evicted earlier)
        }
        if( entry.bound() != 0 || ( text_only && !entry.has_text() ) ) {
            continue;
        }
        if( entry.has( recent_flag ) ) {
            entry.set_flags( static_cast<std::uint8_t>( entry.flags() & ~recent_flag ) );
            save( at, entry );
            continue;
        }
        unindex( at );
        save( at, Entry{} );
        --used_;
        ++evicted_;
        return at;
    }
    return std::nullopt;
}

void ValueCache::unindex( std::uint32_t entry ) noexcept
{
    const Entry gone = load( entry );
    bool found = false;
    std::uint32_t hole = probe( { gone.name(), gone.uid() }, found );
    assert( found );
    // Backward shift: an entry after the hole moves into it unless its home
    // lies after the hole (cyclically) and at or before where it is, which
    // would put it before its home. No tombstones, so lookups stay short.
    std::uint32_t next = hole;
    for( ;; ) {
        next = ( next + 1 ) & slot_mask_;
        const std::uint16_t at = slot_at( next );
        if( at == empty_slot ) {
            break;
        }
        const Entry moved = load( at );
        const std::uint32_t wants = home( moved.name(), moved.uid() );
        const bool after_hole = hole < wants;
        const bool not_after_it = wants <= next;
        const bool stays = hole <= next ? after_hole && not_after_it : after_hole || not_after_it;
        if( !stays ) {
            set_slot( hole, at );
            hole = next;
        }
    }
    set_slot( hole, empty_slot );
}

ValueCache::MaybeEntry ValueCache::text_room( std::uint32_t entry, std::uint32_t length ) noexcept
{
    const std::uint32_t need = header_bytes + length;
    const auto capacity = static_cast<std::uint32_t>( text_.size() );
    if( capacity - text_used_ < need ) {
        compact();
    }
    while( capacity - text_used_ < need ) {
        // The text being replaced is still live. Once dropping it is enough
        // (compacted, its room is its length), it is dropped: the new one
        // then surely fits. Not before: a store that fails keeps the old value.
        Entry replaced = load( entry );
        const std::uint32_t freed = header_bytes + replaced.text().length;
        if( replaced.has_text() && capacity - text_used_ + freed >= need ) {
            replaced.set_flags( static_cast<std::uint8_t>( replaced.flags() & ~valued_flag ) );
            save( entry, replaced );
            compact();
            continue;
        }
        // Still short: unbound entries holding text give theirs up, oldest first.
        if( !evict( entry, true ) ) {
            return std::nullopt;
        }
        compact();
    }
    write_at( text_, text_used_, static_cast<std::uint16_t>( entry ) );
    write_at( text_, text_used_ + 2, static_cast<std::uint16_t>( length ) );
    const std::uint32_t offset = text_used_ + header_bytes;
    text_used_ += need;
    highest_text_ = std::max( highest_text_, text_used_ );
    return offset;
}

void ValueCache::compact() noexcept
{
    // Front to back: a text is kept if its entry still points at it, and
    // slid down over whatever was dropped; its room shrinks to its length.
    std::uint32_t from = 0;
    std::uint32_t to = 0;
    while( from < text_used_ ) {
        const std::uint16_t owner = read_at<std::uint16_t>( text_, from );
        const std::uint16_t room = read_at<std::uint16_t>( text_, from + 2 );
        const std::uint32_t offset = from + header_bytes;
        Entry entry = owner < fresh_ ? load( owner ) : Entry{};
        if( entry.has( used_flag ) && entry.has_text() && entry.text().offset == offset ) {
            const std::uint32_t length = entry.text().length;
            std::memmove( text_.data() + to + header_bytes, text_.data() + offset, length );
            write_at( text_, to, owner );
            write_at( text_, to + 2, static_cast<std::uint16_t>( length ) );
            entry.set_text( { to + header_bytes, length } );
            save( owner, entry );
            to += header_bytes + length;
        }
        from = offset + room;
    }
    text_used_ = to;
}

CacheResult ValueCache::store( const Telegram &telegram, std::span<const std::byte> data ) noexcept
{
    const ValueType type = telegram.type();
    const bool with_data = carries_data( type );
    assert( with_data || data.empty() );
    assert( type != ValueType::uids || data.size() % 4 == 0 );
    const bool long_text = with_data && data.size() > inline_max;
    if( long_text && data.size() > text_.size() / 4 ) {
        ++dropped_;
        return CacheResult::too_large;
    }

    const auto at = entry_for( telegram.address() );
    if( !at ) {
        ++dropped_;
        return CacheResult::dropped;
    }
    Entry entry = load( *at );
    const auto length = static_cast<std::uint32_t>( data.size() );
    DataRef where{};
    if( long_text ) {
        // Rewritten in place when it fits the room the old text had.
        const DataRef old = entry.text();
        const bool fits = entry.has_text() && room_of( old ) >= length;
        if( fits ) {
            where = { old.offset, length };
        } else {
            const auto offset = text_room( *at, length );
            if( !offset ) {
                ++dropped_;
                return CacheResult::dropped;
            }
            where = { *offset, length };
            entry = load( *at ); // compaction may have rewritten it
        }
        std::memcpy( text_.data() + where.offset, data.data(), length );
    }

    // The telegram's bytes, with the entry's bound count and flags where its
    // correlation was.
    const std::array<std::byte, 32> bytes = TelegramBytes::of( telegram );
    std::memcpy( entry.bytes.data() + 14, bytes.data() + 14, 2 );
    std::uint8_t flags = used_flag | recent_flag | valued_flag;
    if( !with_data ) {
        std::memcpy( entry.bytes.data() + 16, bytes.data() + 16, 16 );
    } else if( !long_text ) {
        std::fill( entry.bytes.begin() + 16, entry.bytes.end(), std::byte{ 0 } );
        if( length != 0 ) {
            std::memcpy( entry.bytes.data() + 16, data.data(), length );
        }
        flags = static_cast<std::uint8_t>( flags | ( ( length + 1 ) << inline_shift ) );
    } else {
        std::fill( entry.bytes.begin() + 16, entry.bytes.end(), std::byte{ 0 } );
        entry.set_text( where );
    }
    entry.set_flags( flags );
    save( *at, entry );
    return CacheResult::stored;
}

std::uint32_t ValueCache::room_of( DataRef text ) const noexcept
{
    return read_at<std::uint16_t>( text_, text.offset - header_bytes + 2 );
}

std::optional<CachedValue> ValueCache::find( Address address ) const noexcept
{
    bool found = false;
    const std::uint32_t slot = probe( address, found );
    if( !found ) {
        return std::nullopt;
    }
    const std::uint16_t at = slot_at( slot );
    if( !load( at ).has( valued_flag ) ) {
        return std::nullopt;
    }
    return value_at( at );
}

CachedValue ValueCache::value_at( std::uint32_t at ) const noexcept
{
    const Entry entry = load( at );
    std::array<std::byte, 32> bytes = entry.bytes;
    bytes[12] = std::byte{ 0 }; // the correlation: none
    bytes[13] = std::byte{ 0 };
    if( !carries_data( entry.type() ) ) {
        return CachedValue{ TelegramBytes::from( bytes ), {} };
    }
    std::span<const std::byte> data;
    if( entry.is_inline() ) {
        const std::size_t start = std::size_t{ at } * entry_size + 16;
        data = std::span<const std::byte>( entries_ ).subspan( start, entry.inline_length() );
    } else {
        const DataRef in_text = entry.text();
        data = std::span<const std::byte>( text_ ).subspan( in_text.offset, in_text.length );
    }
    std::fill( bytes.begin() + 16, bytes.end(), std::byte{ 0 } );
    const Telegram shape = TelegramBytes::from( bytes );
    const DataRef where{ 0, static_cast<std::uint32_t>( data.size() ) };
    const Address address{ entry.name(), entry.uid() };
    return CachedValue{ Telegram::data( shape.verb(), address, shape.type(), where ), data };
}

std::uint32_t ValueCache::bindings( Address address ) const noexcept
{
    bool found = false;
    const std::uint32_t slot = probe( address, found );
    return found ? load( slot_at( slot ) ).bound() : 0;
}

std::optional<BoundEntry> ValueCache::bound_from( std::uint32_t from ) const noexcept
{
    for( std::uint32_t at = from; at < fresh_; ++at ) {
        const Entry entry = load( at );
        if( entry.has( used_flag ) && entry.bound() != 0 ) {
            BoundEntry found;
            found.entry = at;
            found.address = { entry.name(), entry.uid() };
            if( entry.has( valued_flag ) ) {
                found.value = value_at( at );
            }
            return found;
        }
    }
    return std::nullopt;
}

bool ValueCache::bind( Address address ) noexcept
{
    const auto at = entry_for( address );
    if( !at ) {
        return false;
    }
    Entry entry = load( *at );
    if( entry.bound() == 0xFF ) {
        return false;
    }
    if( entry.bound() == 0 ) {
        ++bound_;
    }
    entry.set_bound( static_cast<std::uint8_t>( entry.bound() + 1 ) );
    save( *at, entry );
    return true;
}

void ValueCache::unbind( Address address ) noexcept
{
    bool found = false;
    const std::uint32_t slot = probe( address, found );
    assert( found );
    if( !found ) {
        return;
    }
    const std::uint16_t at = slot_at( slot );
    Entry entry = load( at );
    assert( entry.bound() != 0 );
    if( entry.bound() == 0 ) {
        return;
    }
    entry.set_bound( static_cast<std::uint8_t>( entry.bound() - 1 ) );
    if( entry.bound() == 0 ) {
        --bound_;
        // Just shown: the clock passes it once more before taking it.
        entry.set_flags( static_cast<std::uint8_t>( entry.flags() | recent_flag ) );
    }
    save( at, entry );
}

CacheStats ValueCache::stats() const noexcept
{
    CacheStats stats;
    stats.entries = capacity_;
    stats.used = used_;
    stats.bound = bound_;
    stats.text_bytes = static_cast<std::uint32_t>( text_.size() );
    stats.text_used = text_used_;
    stats.highest_text = highest_text_;
    stats.evicted = evicted_;
    stats.dropped = dropped_;
    return stats;
}

} // namespace sq::messaging
