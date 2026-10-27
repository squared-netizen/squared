#include <squared/messaging/model_list.hpp>

#include <algorithm>
#include <cassert>

namespace sq::messaging
{

ModelList::ModelList( Name name, ListSource source, std::span<std::uint32_t> window,
                      std::span<std::uint32_t> scratch ) noexcept
    : source_( source )
    , name_( name )
    , count_name_( Name::from_id( name_hash( ".count", name.id ) ) )
    , window_( window )
    , scratch_( scratch )
{
    assert( source.count != nullptr && source.uid_at != nullptr && source.publish != nullptr );
    assert( !window.empty() && scratch.size() >= window.size() );
}

void ModelList::send_count( const ListOutput &out ) const noexcept
{
    out.send( out.context, Telegram::integer( Verb::state, { count_name_, 0 }, count_ ) );
}

void ModelList::start( const ListOutput &out ) noexcept
{
    count_ = source_.count( source_.source );
    first_ = 0;
    rows_ = 0;
    send_count( out );
}

void ModelList::view( std::uint32_t first, std::uint32_t last, const ListOutput &out ) noexcept
{
    if( last < first ) {
        // A reset: a new widget knows nothing. Forget what was sent; the count.
        show( first, 0, out );
        send_count( out );
        return;
    }
    if( count_ == 0 || first >= count_ ) {
        show( first, 0, out );
        return;
    }
    last = std::min( last, count_ - 1 );
    const auto most = static_cast<std::uint32_t>( window_.size() );
    show( first, std::min( last - first + 1, most ), out );
}

void ModelList::changed( const ListOutput &out ) noexcept
{
    const std::uint32_t count = source_.count( source_.source );
    if( count != count_ ) {
        count_ = count;
        send_count( out );
    }
    // The same view, cut to the new count, read again.
    const std::uint32_t end = std::min( first_ + rows_, count_ );
    show( first_, end > first_ ? end - first_ : 0, out );
}

void ModelList::changed( std::uint32_t uid, const ListOutput &out ) noexcept
{
    const auto shown = window_.first( rows_ );
    if( std::find( shown.begin(), shown.end(), uid ) != shown.end() ) {
        source_.publish( source_.source, out.context, uid );
    }
}

void ModelList::show( std::uint32_t first, std::uint32_t rows, const ListOutput &out ) noexcept
{
    // What was sent, by row: `window_` holds rows first_ .. first_ + rows_.
    const auto sent_at = [this]( std::uint32_t row, std::uint32_t &uid ) {
        if( row < first_ || row - first_ >= rows_ ) {
            return false;
        }
        uid = window_[row - first_];
        return true;
    };
    for( std::uint32_t i = 0; i < rows; ++i ) {
        scratch_[i] = source_.uid_at( source_.source, first + i );
        assert( scratch_[i] != 0 && "uid 0 means none" );
    }
    // A row whose item is not the one sent for it.
    for( std::uint32_t i = 0; i < rows; ++i ) {
        std::uint32_t was = 0;
        if( !sent_at( first + i, was ) || was != scratch_[i] ) {
            const Address row{ name_, first + i };
            out.send( out.context, Telegram::integer( Verb::state, row, scratch_[i] ) );
        }
    }
    // An item new to the view: its fields. Scrolling keeps each item at its
    // row, so that is checked first; a full search only for the others.
    const auto shown = window_.first( rows_ );
    for( std::uint32_t i = 0; i < rows; ++i ) {
        const std::uint32_t uid = scratch_[i];
        std::uint32_t was = 0;
        const bool kept = sent_at( first + i, was ) && was == uid;
        if( !kept && std::find( shown.begin(), shown.end(), uid ) == shown.end() ) {
            source_.publish( source_.source, out.context, uid );
        }
    }
    std::copy_n( scratch_.begin(), rows, window_.begin() );
    first_ = first;
    rows_ = rows;
}

} // namespace sq::messaging
