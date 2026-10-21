#include <squared/messaging/telegraph.hpp>

#include <cassert>

namespace sq::messaging
{

namespace
{

// Rows and their view counts travel as floats: exact up to 2^24.
[[maybe_unused]] constexpr std::uint32_t exact_rows = 1U << 24;

} // namespace

TelegraphLine::TelegraphLine() noexcept
    : to_model_( to_model_slots_, to_model_data_ )
    , to_ui_( to_ui_slots_, to_ui_data_ )
    , cache_( cache_entries_, cache_index_, cache_text_ )
    , pending_( pending_entries_, pending_index_, pending_text_ )
    , model_( *this )
    , ui_( *this )
{}

// ---- the model's end --------------------------------------------------------------------

void Telegraph::tick_every( std::uint32_t milliseconds ) noexcept
{
    line_.tick_ms_ = milliseconds;
}

std::uint32_t Telegraph::tick_interval() const noexcept
{
    return line_.tick_ms_;
}

std::uint32_t Telegraph::pending() const noexcept
{
    return line_.pending_count_;
}

bool Telegraph::quitting() const noexcept
{
    return line_.quitting_;
}

void Telegraph::mark_quitting() noexcept
{
    line_.quitting_ = true;
}

SendResult Telegraph::send_state( const Telegram &telegram,
                                  std::span<const std::byte> data ) noexcept
{
    if( line_.pending_count_ != 0 ) {
        flush();
    }
    // An address with a value held back takes this one there too, so the
    // latest is the one that arrives. Any other address may go first:
    // between addresses, the order of states does not matter.
    if( line_.pending_count_ == 0 || line_.pending_.bindings( telegram.address() ) == 0 ) {
        const RingResult result = line_.to_ui_.push( telegram, data );
        if( result == RingResult::sent ) {
            return SendResult::sent;
        }
        if( result == RingResult::too_large ) {
            return SendResult::too_large;
        }
    }
    return hold( telegram, data );
}

SendResult Telegraph::hold( const Telegram &telegram, std::span<const std::byte> data ) noexcept
{
    const Address address = telegram.address();
    const bool fresh = line_.pending_.bindings( address ) == 0;
    // Bound, so the table never evicts it before it is sent.
    if( fresh && !line_.pending_.bind( address ) ) {
        return SendResult::full; // every entry holds a state still to send: the model waits (6.6c)
    }
    const CacheResult stored = line_.pending_.store( telegram, data );
    if( stored != CacheResult::stored ) {
        if( fresh ) {
            line_.pending_.unbind( address );
        }
        return stored == CacheResult::too_large ? SendResult::too_large : SendResult::full;
    }
    if( fresh ) {
        ++line_.pending_count_;
    }
    return SendResult::pending;
}

SendResult Telegraph::send_event( const Telegram &telegram,
                                  std::span<const std::byte> data ) noexcept
{
    // An event never overtakes the states sent before it.
    if( line_.pending_count_ != 0 && !flush() ) {
        return SendResult::full;
    }
    switch( line_.to_ui_.push( telegram, data ) ) {
        case RingResult::sent:
            return SendResult::sent;
        case RingResult::too_large:
            return SendResult::too_large;
        case RingResult::full:
            break;
    }
    return SendResult::full;
}

bool Telegraph::flush() noexcept
{
    std::uint32_t from = 0;
    while( line_.pending_count_ != 0 ) {
        const auto held = line_.pending_.bound_from( from );
        if( !held ) {
            assert( false && "a pending count with nothing bound" );
            line_.pending_count_ = 0;
            break;
        }
        if( held->value ) {
            const RingResult result = line_.to_ui_.push( held->value->telegram, held->value->data );
            if( result == RingResult::full ) {
                return false;
            }
        }
        line_.pending_.unbind( held->address );
        --line_.pending_count_;
        from = held->entry + 1;
    }
    return true;
}

ListOutput Telegraph::output() noexcept
{
    return { this, []( void *context, const Telegram &telegram ) noexcept {
                auto *self = static_cast<Telegraph *>( context );
                static_cast<void>( self->send_state( telegram, {} ) );
            } };
}

ModelList *Telegraph::list_named( Name rows ) noexcept
{
    for( auto &list : line_.lists_ ) {
        if( list && list->name() == rows ) {
            return &*list;
        }
    }
    return nullptr;
}

bool Telegraph::add_list( Name rows, ListSource source ) noexcept
{
    if( list_named( rows ) != nullptr ) {
        return false;
    }
    for( std::size_t i = 0; i < line_.lists_.size(); ++i ) {
        std::optional<ModelList> &slot = line_.lists_[i];
        if( !slot.has_value() ) {
            ModelList &list = slot.emplace( rows, source, line_.windows_[i], line_.scratch_ );
            line_.view_names_[i] = Name::from_id( name_hash( ".view", rows.id ) );
            list.start( output() );
            return true;
        }
    }
    return false;
}

void Telegraph::changed_list( Name rows ) noexcept
{
    if( ModelList *list = list_named( rows ) ) {
        list->changed( output() );
    }
}

void Telegraph::changed_item( Name rows, std::uint32_t uid ) noexcept
{
    if( ModelList *list = list_named( rows ) ) {
        list->changed( uid, output() );
    }
}

bool Telegraph::answer_view( const Received &received ) noexcept
{
    const Telegram &t = received.telegram;
    if( t.verb() != Verb::do_ || t.type() != ValueType::vec2 ) {
        return false;
    }
    for( std::size_t i = 0; i < line_.lists_.size(); ++i ) {
        std::optional<ModelList> &slot = line_.lists_[i];
        if( slot.has_value() && line_.view_names_[i] == t.address().name ) {
            const std::array<float, 4> rows = t.as_floats();
            const auto first = static_cast<std::uint32_t>( rows[0] );
            const auto last = static_cast<std::uint32_t>( rows[1] );
            slot->view( first, last, output() );
            return true;
        }
    }
    return false;
}

Telegraph::Signals Telegraph::take_signals() noexcept
{
    const std::uint32_t bits = line_.signals_.exchange( 0, std::memory_order_acq_rel );
    Signals signals;
    signals.low_memory = ( bits & TelegraphLine::low_memory_bit ) != 0;
    signals.background = ( bits & TelegraphLine::background_bit ) != 0;
    signals.foreground = ( bits & TelegraphLine::foreground_bit ) != 0;
    signals.quit = ( bits & TelegraphLine::quit_bit ) != 0;
    signals.visible = line_.visible_.load( std::memory_order_acquire );
    signals.elapsed = line_.elapsed_.load( std::memory_order_acquire );
    return signals;
}

std::optional<Received> Telegraph::next_from_ui() const noexcept
{
    return line_.to_model_.front();
}

void Telegraph::pop_from_ui() noexcept
{
    line_.to_model_.pop();
}

// ---- the UI's end --------------------------------------------------------------------------

SendResult TelegraphUi::push( const Telegram &telegram, std::span<const std::byte> data ) noexcept
{
    switch( line_.to_model_.push( telegram, data ) ) {
        case RingResult::sent:
            return SendResult::sent;
        case RingResult::too_large:
            return SendResult::too_large;
        case RingResult::full:
            break;
    }
    return SendResult::full;
}

DoResult TelegraphUi::request( const Telegram &telegram, std::span<const std::byte> data ) noexcept
{
    // 1 to 65,535, then round again: 0 means none.
    std::uint16_t next = static_cast<std::uint16_t>( line_.correlation_ + 1 );
    if( next == 0 ) {
        next = 1;
    }
    const SendResult result = push( telegram.with_correlation( next ), data );
    if( result != SendResult::sent ) {
        return { result, 0 };
    }
    line_.correlation_ = next;
    return { result, next };
}

SendResult TelegraphUi::send_view( Name rows, RowSpan span ) noexcept
{
    assert( span.rows != 0 && span.last() < exact_rows && "rows travel as floats" );
    const Name view = Name::from_id( name_hash( ".view", rows.id ) );
    const std::array<float, 2> first_last{ static_cast<float>( span.first ),
                                           static_cast<float>( span.last() ) };
    return push( Telegram::floats( Verb::do_, { view, 0 }, first_last ), {} );
}

std::uint32_t TelegraphUi::drain() noexcept
{
    struct NoEvents
    {};
    NoEvents none;
    return drain( none );
}

std::optional<Received> TelegraphUi::next_from_model() const noexcept
{
    return line_.to_ui_.front();
}

void TelegraphUi::pop_from_model() noexcept
{
    line_.to_ui_.pop();
}

void TelegraphUi::take_state( const Received &received ) noexcept
{
    // A value the cache cannot keep is counted there (the `telegraph` command).
    static_cast<void>( line_.cache_.store( received.telegram, received.data ) );
}

std::optional<CachedValue> TelegraphUi::find( Address address ) const noexcept
{
    return line_.cache_.find( address );
}

bool TelegraphUi::bind( Address address ) noexcept
{
    return line_.cache_.bind( address );
}

void TelegraphUi::unbind( Address address ) noexcept
{
    line_.cache_.unbind( address );
}

void TelegraphUi::signal( std::uint32_t bit ) noexcept
{
    line_.signals_.fetch_or( bit, std::memory_order_acq_rel );
}

void TelegraphUi::background() noexcept
{
    line_.visible_.store( false, std::memory_order_release );
    signal( TelegraphLine::background_bit );
}

void TelegraphUi::foreground( std::uint64_t elapsed_milliseconds ) noexcept
{
    line_.elapsed_.store( elapsed_milliseconds, std::memory_order_release );
    line_.visible_.store( true, std::memory_order_release );
    signal( TelegraphLine::foreground_bit );
}

void TelegraphUi::low_memory() noexcept
{
    signal( TelegraphLine::low_memory_bit );
}

void TelegraphUi::quit() noexcept
{
    signal( TelegraphLine::quit_bit );
}

CacheStats TelegraphUi::cache_stats() const noexcept
{
    return line_.cache_.stats();
}

RingStats TelegraphUi::to_model_stats() const noexcept
{
    return line_.to_model_.stats();
}

RingStats TelegraphUi::to_ui_stats() const noexcept
{
    return line_.to_ui_.stats();
}

} // namespace sq::messaging
