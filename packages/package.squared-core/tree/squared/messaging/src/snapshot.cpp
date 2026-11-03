module sq.messaging;

import std;

namespace sq::messaging
{

// ---- the model's end: naming, filling, publishing ------------------------------------------

detail::SnapshotChannel *Telegraph::snapshot_named( Name name, bool add ) noexcept
{
    // The model's thread is the only one that adds, so its own count needs
    // no ordering; the release below is for the UI.
    const std::uint32_t count = line_.snapshots_count_.load( std::memory_order_relaxed );
    for( std::uint32_t i = 0; i < count; ++i ) {
        if( line_.snapshot_names_[i] == name ) {
            return &line_.snapshot_channels_[i];
        }
    }
    if( !add || count == telegraph_size::snapshots ) {
        return nullptr;
    }
    line_.snapshot_names_[count] = name;
    line_.snapshots_count_.store( count + 1, std::memory_order_release );
    return &line_.snapshot_channels_[count];
}

std::span<std::byte> Telegraph::snapshot( const SnapshotKey &key, std::size_t bytes )
{
    detail::SnapshotChannel *channel = snapshot_named( key.name(), true );
    if( channel == nullptr ) {
        return {};
    }
    return channel->back( bytes );
}

bool Telegraph::publish( const SnapshotKey &key, std::size_t bytes ) noexcept
{
    detail::SnapshotChannel *channel = snapshot_named( key.name(), false );
    if( channel == nullptr ) {
        return false;
    }
    channel->publish( bytes );
    wake_ui();
    return true;
}

// ---- the UI's end: the newest ---------------------------------------------------------------

std::optional<SnapshotView> TelegraphUi::snapshot( const SnapshotKey &key ) noexcept
{
    note( key );
    const std::uint32_t count = line_.snapshots_count_.load( std::memory_order_acquire );
    for( std::uint32_t i = 0; i < count; ++i ) {
        if( line_.snapshot_names_[i] == key.name() ) {
            return line_.snapshot_channels_[i].latest();
        }
    }
    return std::nullopt;
}

} // namespace sq::messaging
