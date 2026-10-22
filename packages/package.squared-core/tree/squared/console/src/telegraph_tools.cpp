#include <squared/console/telegraph_tools.hpp>
#include <squared/messaging/telegram_text.hpp>
#include <squared/time/now.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

namespace sq::console
{

namespace
{

using messaging::BoundEntry;
using messaging::TelegraphUi;
using messaging::write_telegram;

constexpr const char *telegraph_help =
    "telegraph [state | tap | record | replay]: figures, values, and the dev tools";
constexpr std::string_view default_file = "replay.txt";
// At most this many replayed telegrams a frame, however many are due.
constexpr int replay_per_update = 64;

void print_usage( Console &console ) noexcept
{
    console.print( "telegraph: each capacity and its highest fill", Level::error );
    console.print( "telegraph state: every value the UI holds", Level::error );
    console.print( "telegraph tap [on | off]: every telegram as a line", Level::error );
    console.print( "telegraph record [file] | record stop: what the UI sends", Level::error );
    console.print( "telegraph replay [file] [fast] | replay stop: send it again", Level::error );
}

void print_ring( Console &console, const char *direction,
                 const messaging::RingStats &ring ) noexcept
{
    console.printf( Level::info, "%-13s %u of %u telegrams at most, %u of %u data bytes", direction,
                    ring.highest_slots, ring.slots, ring.highest_data, ring.data_bytes );
}

void print_stats( Console &console, const TelegraphUi &ui ) noexcept
{
    print_ring( console, "to the model:", ui.to_model_stats() );
    print_ring( console, "to the UI:", ui.to_ui_stats() );
    const messaging::CacheStats cache = ui.cache_stats();
    console.printf( Level::info, "%-13s %u of %u entries, %u bound", "cache:", cache.used,
                    cache.entries, cache.bound );
    console.printf( Level::info, "%-13s text %u of %u bytes, %u at most", "", cache.text_used,
                    cache.text_bytes, cache.highest_text );
    console.printf( Level::info, "%-13s %u evicted for room, %u dropped", "", cache.evicted,
                    cache.dropped );
    const messaging::ModelStats model = ui.model_stats();
    const auto pending = static_cast<unsigned>( messaging::telegraph_size::pending );
    const auto lists = static_cast<unsigned>( messaging::telegraph_size::lists );
    console.printf( Level::info, "%-13s %u now, %u at most, of %u", "held back:", model.held,
                    model.highest_held, pending );
    console.printf( Level::info, "%-13s %u of %u", "lists:", model.lists, lists );
    const messaging::WaitStats waits = ui.wait_stats();
    const Level level = waits.full != 0 ? Level::warning : Level::info;
    console.printf( level, "%-13s waited %u times, refused %u (full)", "model sends:", waits.waits,
                    waits.full );
}

// The order `telegraph state` prints in: by name id, then uid. The same names
// give the same ids on every run, so two dumps compare line for line.
bool before( messaging::Address left, messaging::Address right ) noexcept
{
    if( left.name.id != right.name.id ) {
        return left.name.id < right.name.id;
    }
    return left.uid < right.uid;
}

// The valued entry that comes next after `previous` (all of them, from the
// first, when there is none): a selection walk, O(n) a line and nothing
// allocated, for a command that runs when asked.
std::optional<BoundEntry> next_value( const TelegraphUi &ui,
                                      const std::optional<messaging::Address> &previous ) noexcept
{
    std::optional<BoundEntry> best;
    std::uint32_t from = 0;
    for( ;; ) {
        const std::optional<BoundEntry> found = ui.cached_from( from );
        if( !found.has_value() ) {
            return best;
        }
        from = found->entry + 1;
        const bool valued = found->value.has_value();
        const bool after = !previous.has_value() || before( *previous, found->address );
        if( valued && after && ( !best.has_value() || before( found->address, best->address ) ) ) {
            best = found;
        }
    }
}

void print_state( Console &console, const TelegraphTools &tools ) noexcept
{
    const TelegraphUi &ui = tools.ui();
    const messaging::NameTable &names = tools.names();
    std::array<char, format_bytes> line{};
    std::uint32_t shown = 0;
    std::optional<messaging::Address> previous;
    for( ;; ) {
        const std::optional<BoundEntry> value = next_value( ui, previous );
        if( !value.has_value() || !value->value.has_value() ) {
            break;
        }
        previous = value->address;
        const messaging::CachedValue &cached = *value->value;
        const std::size_t length = write_telegram( cached.telegram, cached.data, names, line );
        if( length != 0 ) {
            console.print( { line.data(), length } );
        } else {
            const std::size_t address = names.format( value->address, line );
            console.printf( Level::warning, "state %.*s: %zu bytes of value, longer than a line",
                            static_cast<int>( address ), line.data(), cached.data.size() );
        }
        ++shown;
    }
    std::uint32_t waiting = 0;
    std::uint32_t from = 0;
    for( ;; ) {
        const std::optional<BoundEntry> found = ui.cached_from( from );
        if( !found.has_value() ) {
            break;
        }
        from = found->entry + 1;
        if( !found->value.has_value() ) {
            ++waiting;
        }
    }
    console.printf( Level::info, "telegraph: %u values; %u bound, with no value yet", shown,
                    waiting );
}

// A plain file name, as `push` takes them: no directory in it.
bool plain_name( std::string_view name ) noexcept
{
    const bool dots = name == "." || name == "..";
    return !name.empty() && name.size() <= 64 && !dots &&
           name.find_first_of( "/\\" ) == std::string_view::npos;
}

bool write_all( int fd, std::string_view text ) noexcept
{
    while( !text.empty() ) {
        const ssize_t wrote = ::write( fd, text.data(), text.size() );
        if( wrote < 0 && errno == EINTR ) {
            continue;
        }
        if( wrote <= 0 ) {
            return false;
        }
        text.remove_prefix( static_cast<std::size_t>( wrote ) );
    }
    return true;
}

void record_command( Console &console, TelegraphTools &tools, const Args &args ) noexcept
{
    if( args.size() == 3 && args.text( 2 ) == "stop" ) {
        if( !tools.recording() ) {
            console.print( "telegraph: not recording" );
            return;
        }
        const std::uint32_t written = tools.stop_recording();
        console.printf( Level::info, "telegraph: recorded %u telegrams", written );
        return;
    }
    const std::string_view file = args.size() >= 3 ? args.text( 2 ) : default_file;
    if( !tools.record( file ) ) {
        console.printf( Level::error, "telegraph: cannot record to %.*s (a plain name, in files/)",
                        static_cast<int>( file.size() ), file.data() );
        return;
    }
    console.printf( Level::info, "telegraph: recording what the UI sends to files/%.*s",
                    static_cast<int>( file.size() ), file.data() );
}

void replay_command( Console &console, TelegraphTools &tools, const Args &args ) noexcept
{
    if( args.size() == 3 && args.text( 2 ) == "stop" ) {
        tools.stop_replay();
        return;
    }
    std::string_view file = default_file;
    bool fast = false;
    for( std::size_t word = 2; word < args.size(); ++word ) {
        if( args.text( word ) == "fast" ) {
            fast = true;
        } else {
            file = args.text( word );
        }
    }
    if( !tools.replay( file, fast ) ) {
        console.printf( Level::error, "telegraph: cannot replay files/%.*s",
                        static_cast<int>( file.size() ), file.data() );
        return;
    }
    console.printf( Level::info, "telegraph: replaying files/%.*s%s",
                    static_cast<int>( file.size() ), file.data(), fast ? ", fast" : "" );
}

void telegraph( Console &console, const Args &args, void *context ) noexcept
{
    auto &tools = *static_cast<TelegraphTools *>( context );
    const std::string_view word = args.size() > 1 ? args.text( 1 ) : std::string_view{};
    if( args.size() == 1 ) {
        print_stats( console, tools.ui() );
    } else if( word == "state" && args.size() == 2 ) {
        print_state( console, tools );
    } else if( word == "tap" && args.size() <= 3 ) {
        const std::string_view how = args.size() == 3 ? args.text( 2 ) : std::string_view{};
        if( how == "on" || how == "off" ) {
            tools.tap( how == "on" );
        }
        console.printf( Level::info, "telegraph: the tap is %s", tools.tapping() ? "on" : "off" );
    } else if( word == "record" && args.size() <= 3 ) {
        record_command( console, tools, args );
    } else if( word == "replay" && args.size() <= 4 ) {
        replay_command( console, tools, args );
    } else {
        print_usage( console );
    }
}

} // namespace

TelegraphTools::TelegraphTools( messaging::TelegraphUi &ui, const char *directory ) noexcept
    : ui_( ui )
    , directory_( directory )
    , names_( name_storage_ )
    , value_( value_storage_ )
{
    ui_.set_names( &names_ );
}

TelegraphTools::~TelegraphTools()
{
    ui_.set_tap( nullptr, nullptr );
    ui_.set_names( nullptr );
    if( record_fd_ >= 0 ) {
        ::close( record_fd_ );
    }
    if( replay_fd_ >= 0 ) {
        ::close( replay_fd_ );
    }
}

std::size_t TelegraphTools::add_commands( Console &console ) noexcept
{
    console_ = &console;
    std::size_t refused = 0;
    if( console.add( "telegraph", telegraph_help, telegraph, this ) !=
        Commands::AddResult::added ) {
        ++refused;
    }
    if( console.add_to_group( "telegraph", "telegraph" ) != Groups::AddResult::added ) {
        ++refused;
    }
    if( console.add_to_group( "state", "telegraph state" ) != Groups::AddResult::added ) {
        ++refused;
    }
    return refused;
}

void TelegraphTools::set_sink( Sink sink, void *context ) noexcept
{
    sink_ = sink;
    sink_context_ = context;
}

void TelegraphTools::say( Level level, std::string_view line ) noexcept
{
    if( console_ != nullptr ) {
        console_->print( line, level );
    }
}

void TelegraphTools::attach_tap() noexcept
{
    // Installed only while something listens: no call a telegram otherwise.
    const bool listening = tapping_ || record_fd_ >= 0;
    ui_.set_tap( listening ? tapped : nullptr, this );
}

void TelegraphTools::tap( bool on ) noexcept
{
    tapping_ = on;
    attach_tap();
}

void TelegraphTools::tapped( void *context, messaging::TapDirection direction,
                             const messaging::Telegram &telegram,
                             std::span<const std::byte> data ) noexcept
{
    auto &self = *static_cast<TelegraphTools *>( context );
    const bool to_model = direction == messaging::TapDirection::to_model;
    std::span<char> out( self.line_ );
    if( self.tapping_ ) {
        const std::string_view prefix = to_model ? "ui>model " : "model>ui ";
        std::copy( prefix.begin(), prefix.end(), out.begin() );
        const std::size_t length =
            write_telegram( telegram, data, self.names_, out.subspan( prefix.size() ) );
        const std::string_view line( out.data(), prefix.size() + length );
        const std::string_view shown =
            length != 0 ? line : "telegraph: a telegram too long to show";
        if( self.sink_ != nullptr ) {
            self.sink_( self.sink_context_, shown );
        } else {
            self.say( Level::info, shown );
        }
    }
    if( self.record_fd_ >= 0 && to_model ) {
        // "<milliseconds since the start> <the telegram's line>"
        const auto since = std::chrono::duration_cast<std::chrono::milliseconds>(
            time::now() - self.record_start_ );
        const auto [end, error] = std::to_chars( out.data(), out.data() + 24, since.count() );
        static_cast<void>( error );
        auto stamp = static_cast<std::size_t>( end - out.data() );
        out[stamp] = ' ';
        ++stamp;
        const std::size_t length = write_telegram( telegram, data, self.names_,
                                                   out.subspan( stamp, out.size() - stamp - 1 ) );
        if( length == 0 ) {
            self.say( Level::warning, "telegraph: a telegram too long to record was left out" );
            return;
        }
        out[stamp + length] = '\n';
        if( write_all( self.record_fd_, { out.data(), stamp + length + 1 } ) ) {
            ++self.recorded_;
        }
    }
}

bool TelegraphTools::path_of( std::string_view file ) noexcept
{
    if( directory_ == nullptr || !plain_name( file ) ) {
        return false;
    }
    const std::string_view directory( directory_ );
    if( directory.size() + file.size() + 2 > path_.size() ) {
        return false;
    }
    std::copy( directory.begin(), directory.end(), path_.begin() );
    path_[directory.size()] = '/';
    const auto name_at = path_.begin() + static_cast<std::ptrdiff_t>( directory.size() + 1 );
    const auto end = std::copy( file.begin(), file.end(), name_at );
    *end = '\0';
    return true;
}

bool TelegraphTools::record( std::string_view file ) noexcept
{
    if( !path_of( file ) ) {
        return false;
    }
    if( record_fd_ >= 0 ) {
        static_cast<void>( stop_recording() );
    }
    record_fd_ = ::open( path_.data(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600 );
    if( record_fd_ < 0 ) {
        return false;
    }
    record_start_ = time::now();
    recorded_ = 0;
    attach_tap();
    return true;
}

std::uint32_t TelegraphTools::stop_recording() noexcept
{
    if( record_fd_ >= 0 ) {
        ::close( record_fd_ );
        record_fd_ = -1;
    }
    attach_tap();
    return recorded_;
}

bool TelegraphTools::replay( std::string_view file, bool fast ) noexcept
{
    if( !path_of( file ) ) {
        return false;
    }
    if( replay_fd_ >= 0 ) {
        ::close( replay_fd_ );
    }
    replay_fd_ = ::open( path_.data(), O_RDONLY | O_CLOEXEC );
    if( replay_fd_ < 0 ) {
        return false;
    }
    replay_fast_ = fast;
    replay_started_ = false;
    replay_end_ = false;
    pending_.reset();
    read_start_ = 0;
    read_end_ = 0;
    replay_line_ = 0;
    replayed_ = 0;
    unread_ = 0;
    return true;
}

void TelegraphTools::stop_replay() noexcept
{
    if( replay_fd_ < 0 ) {
        say( Level::info, "telegraph: not replaying" );
        return;
    }
    ::close( replay_fd_ );
    replay_fd_ = -1;
    if( console_ != nullptr ) {
        console_->printf( Level::info, "telegraph: replay stopped after %u sent", replayed_ );
    }
}

void TelegraphTools::finish_replay() noexcept
{
    ::close( replay_fd_ );
    replay_fd_ = -1;
    if( console_ != nullptr ) {
        const Level level = unread_ != 0 ? Level::warning : Level::info;
        console_->printf( level, "telegraph: replay done, %u sent, %u lines not sent", replayed_,
                          unread_ );
    }
}

bool TelegraphTools::next_line( std::string_view &line ) noexcept
{
    bool skipping = false; // inside a line longer than the buffer
    for( ;; ) {
        const std::string_view buffered( read_.data() + read_start_, read_end_ - read_start_ );
        const std::size_t newline = buffered.find( '\n' );
        if( newline != std::string_view::npos ) {
            read_start_ += newline + 1;
            ++replay_line_;
            if( skipping ) {
                skipping = false;
                ++unread_;
                continue;
            }
            line = buffered.substr( 0, newline );
            return true;
        }
        if( replay_end_ ) {
            read_start_ = read_end_;
            if( buffered.empty() || skipping ) {
                return false;
            }
            ++replay_line_;
            line = buffered;
            return true;
        }
        // Keep what is buffered at the front, and read on after it.
        std::memmove( read_.data(), buffered.data(), buffered.size() );
        read_start_ = 0;
        read_end_ = buffered.size();
        if( read_end_ == read_.size() ) {
            read_end_ = 0; // a line longer than the buffer: left out
            skipping = true;
        }
        const ssize_t got =
            ::read( replay_fd_, read_.data() + read_end_, read_.size() - read_end_ );
        if( got < 0 && errno == EINTR ) {
            continue;
        }
        if( got <= 0 ) {
            replay_end_ = true;
        } else {
            read_end_ += static_cast<std::size_t>( got );
        }
    }
}

void TelegraphTools::update( time::TimePoint now ) noexcept
{
    if( replay_fd_ < 0 ) {
        return;
    }
    if( !replay_started_ ) {
        replay_start_ = now; // the first line's time counts from the first update
        replay_started_ = true;
    }
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>( now - replay_start_ ).count();
    for( int sent = 0; sent < replay_per_update; ) {
        while( !pending_.has_value() ) {
            std::string_view line;
            if( !next_line( line ) ) {
                finish_replay();
                return;
            }
            if( line.empty() || line.front() == '#' ) {
                continue;
            }
            // "<milliseconds> <telegram>"
            std::int64_t due = 0;
            const auto [end, error] =
                std::from_chars( line.data(), line.data() + line.size(), due );
            const auto stamp = static_cast<std::size_t>( end - line.data() );
            value_.clear();
            const bool spaced = error == std::errc{} && stamp < line.size() && line[stamp] == ' ';
            const std::optional<messaging::Telegram> read =
                spaced ? messaging::read_telegram( line.substr( stamp + 1 ), value_ )
                       : std::nullopt;
            if( !read.has_value() ) {
                ++unread_;
                if( console_ != nullptr ) {
                    console_->printf( Level::warning, "telegraph: replay line %u is not one",
                                      replay_line_ );
                }
                continue;
            }
            pending_ = *read;
            pending_due_ms_ = due;
        }
        if( !replay_fast_ && elapsed < pending_due_ms_ ) {
            return; // not yet
        }
        const messaging::SendResult result = ui_.send( *pending_, value_.bytes() );
        if( result == messaging::SendResult::full ) {
            return; // the ring is full: again next frame
        }
        if( result == messaging::SendResult::sent ) {
            ++replayed_;
        } else {
            ++unread_;
        }
        pending_.reset();
        ++sent;
    }
}

} // namespace sq::console
