#include <squared/console/bridge.hpp>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstring>

namespace sq::console
{

namespace
{

namespace protocol = bridge_protocol;

/// File chunks sent in one go before the bridge waits for the socket again,
/// so a large file does not hold up the frame: 64 KB.
constexpr int chunks_per_turn = 16;

[[nodiscard]] bool good_secret( std::string_view secret ) noexcept
{
    if( secret.size() < protocol::secret_min || secret.size() > protocol::secret_max ) {
        return false;
    }
    return std::all_of( secret.begin(), secret.end(), []( char c ) {
        return ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
    } );
}

/// Equal, in a time that does not say how much of `given` matched.
[[nodiscard]] bool same_secret( std::string_view given, std::string_view expected ) noexcept
{
    unsigned difference = given.size() == expected.size() ? 0U : 1U;
    for( std::size_t i = 0; i < expected.size(); ++i ) {
        const char g = i < given.size() ? given[i] : '\0';
        difference |= static_cast<unsigned>( static_cast<unsigned char>( g ) ^
                                             static_cast<unsigned char>( expected[i] ) );
    }
    return difference == 0;
}

[[nodiscard]] sockaddr_in loopback( std::uint16_t port ) noexcept
{
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons( port );
    address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    return address;
}

/// The whole of `bytes` now, or false: for the few lines written outside the
/// output buffer (to a client being replaced).
bool write_now( int fd, std::string_view bytes ) noexcept
{
    const ssize_t wrote = ::send( fd, bytes.data(), bytes.size(), MSG_NOSIGNAL );
    return wrote == static_cast<ssize_t>( bytes.size() );
}

/// All of `bytes` to a file; false on any failure.
bool write_file( int fd, std::string_view bytes ) noexcept
{
    while( !bytes.empty() ) {
        const ssize_t wrote = ::write( fd, bytes.data(), bytes.size() );
        if( wrote < 0 && errno == EINTR ) {
            continue;
        }
        if( wrote <= 0 ) {
            return false;
        }
        bytes.remove_prefix( static_cast<std::size_t>( wrote ) );
    }
    return true;
}

[[nodiscard]] bool parse_size( std::string_view text, std::uint64_t &out ) noexcept
{
    const auto [end, error] = std::from_chars( text.data(), text.data() + text.size(), out );
    return error == std::errc{} && end == text.data() + text.size() && !text.empty();
}

} // namespace

BridgeStatus Bridge::open( const BridgeConfig &config, const BridgeHandlers &handlers ) noexcept
{
    if( listener_ >= 0 ) {
        return BridgeStatus::already_open;
    }
    if( !good_secret( config.secret ) ) {
        return BridgeStatus::bad_secret;
    }
    const int fd = ::socket( AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0 );
    if( fd < 0 ) {
        return BridgeStatus::refused;
    }
    const int yes = 1;
    ::setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( yes ) );
    sockaddr_in address = loopback( config.port );
    socklen_t length = sizeof( address );
    if( ::bind( fd, reinterpret_cast<const sockaddr *>( &address ), sizeof( address ) ) != 0 ||
        ::listen( fd, 2 ) != 0 ||
        ::getsockname( fd, reinterpret_cast<sockaddr *>( &address ), &length ) != 0 ) {
        ::close( fd );
        return BridgeStatus::refused;
    }
    listener_ = fd;
    port_ = ntohs( address.sin_port );
    handlers_ = handlers;
    std::copy( config.secret.begin(), config.secret.end(), secret_.begin() );
    secret_size_ = config.secret.size();
    const std::size_t directory =
        std::min( config.push_directory.size(), push_directory_.size() - 1 );
    std::copy_n( config.push_directory.begin(), directory, push_directory_.begin() );
    push_directory_[directory] = '\0';
    dropped_ = 0;
    unreported_ = 0;
    if( handlers_.watch != nullptr ) {
        handlers_.watch( handlers_.context, listener_, true, false );
    }
    if( config.announce ) {
        // One datagram, fire and forget: a client waiting on that port learns
        // the app is up without asking again and again.
        const int udp = ::socket( AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0 );
        if( udp >= 0 ) {
            const sockaddr_in to = loopback( port_ );
            ::sendto( udp, protocol::announce.data(), protocol::announce.size(), MSG_NOSIGNAL,
                      reinterpret_cast<const sockaddr *>( &to ), sizeof( to ) );
            ::close( udp );
        }
    }
    return BridgeStatus::listening;
}

void Bridge::close_socket( int &fd ) noexcept
{
    if( fd < 0 ) {
        return;
    }
    if( handlers_.watch != nullptr ) {
        handlers_.watch( handlers_.context, fd, false, false );
    }
    ::close( fd );
    fd = -1;
}

void Bridge::abandon_push() noexcept
{
    if( push_ < 0 ) {
        return;
    }
    // Half a file is not the file: the part is removed.
    ::close( push_ );
    push_ = -1;
    std::array<char, 400> part{};
    if( std::snprintf( part.data(), part.size(), "%s/%s.part", push_directory_.data(),
                       push_name_.data() ) > 0 ) {
        ::unlink( part.data() );
    }
}

void Bridge::end_transfers() noexcept
{
    if( file_ >= 0 ) {
        ::close( file_ );
        file_ = -1;
    }
    abandon_push();
    file_left_ = 0;
    push_left_ = 0;
    dones_after_file_ = 0;
}

void Bridge::close() noexcept
{
    end_transfers();
    close_socket( pending_ );
    close_socket( client_ );
    close_socket( listener_ );
    output_size_ = 0;
    command_size_ = 0;
    hello_size_ = 0;
    writing_ = false;
    cutting_ = false;
    std::fill( secret_.begin(), secret_.end(), '\0' );
    secret_size_ = 0;
}

bool Bridge::ready( int fd, bool readable, bool writable ) noexcept
{
    if( fd < 0 ) {
        return false;
    }
    if( fd == listener_ ) {
        accept_clients();
        return true;
    }
    if( fd == pending_ ) {
        return readable ? read_hello() : true;
    }
    if( fd == client_ ) {
        if( writable ) {
            flush();
            if( client_ != fd ) {
                return false;
            }
        }
        return readable ? read_commands() : true;
    }
    return false;
}

void Bridge::accept_clients() noexcept
{
    for( ;; ) {
        const int fd = ::accept4( listener_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC );
        if( fd < 0 ) {
            return; // EAGAIN: none left; anything else: try at the next event
        }
        // The newest connection is the one that may give the secret: an
        // earlier one that has not is dropped.
        close_socket( pending_ );
        pending_ = fd;
        hello_size_ = 0;
        if( handlers_.watch != nullptr ) {
            handlers_.watch( handlers_.context, pending_, true, false );
        }
    }
}

bool Bridge::read_hello() noexcept
{
    const ssize_t got =
        ::recv( pending_, hello_.data() + hello_size_, hello_.size() - hello_size_, 0 );
    if( got < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) ) {
        return true;
    }
    if( got <= 0 ) {
        close_socket( pending_ );
        return false;
    }
    hello_size_ += static_cast<std::size_t>( got );
    const std::string_view held( hello_.data(), hello_size_ );
    const auto end = held.find( '\n' );
    if( end == std::string_view::npos ) {
        if( hello_size_ == hello_.size() ) {
            close_socket( pending_ ); // no newline in 160 bytes: not a client of ours
            return false;
        }
        return true;
    }
    std::string_view line = held.substr( 0, end );
    if( !line.empty() && line.back() == '\r' ) {
        line.remove_suffix( 1 );
    }
    bool replay = true;
    if( line.size() >= protocol::quiet.size() &&
        line.substr( line.size() - protocol::quiet.size() ) == protocol::quiet ) {
        line.remove_suffix( protocol::quiet.size() );
        replay = false;
    }
    const bool hello = line.substr( 0, protocol::hello.size() ) == protocol::hello;
    const std::string_view given =
        hello ? line.substr( protocol::hello.size() ) : std::string_view{};
    if( !hello || !same_secret( given, { secret_.data(), secret_size_ } ) ) {
        close_socket( pending_ ); // no answer: a wrong secret learns nothing
        return false;
    }
    // Bytes after the hello are the first commands; kept aside while the
    // hello buffer is reused.
    std::array<char, 160> rest{};
    const std::size_t rest_size = hello_size_ - ( end + 1 );
    std::copy_n( hello_.begin() + static_cast<std::ptrdiff_t>( end + 1 ), rest_size, rest.begin() );
    const int fd = pending_;
    take_over();
    if( handlers_.connected != nullptr ) {
        handlers_.connected( handlers_.context, replay );
    }
    if( client_ == fd && rest_size != 0 ) {
        static_cast<void>( consume( { rest.data(), rest_size } ) );
    }
    return client_ == fd; // the same socket, now the client's, unless a command closed it
}

void Bridge::take_over() noexcept
{
    if( client_ >= 0 ) {
        // Told directly, past whatever is still queued for it: it is going.
        static_cast<void>( write_now( client_, "w bridge: another connection took over\n" ) );
        close_socket( client_ );
    }
    end_transfers();
    client_ = pending_;
    pending_ = -1;
    hello_size_ = 0;
    output_size_ = 0;
    output_head_ = 0;
    command_size_ = 0;
    cutting_ = false;
    writing_ = false;
    unreported_ = 0;
    watch_client();
    queue( '\0', protocol::ok );
}

bool Bridge::read_commands() noexcept
{
    const int fd = client_;
    std::array<char, protocol::chunk_bytes> in{};
    for( ;; ) {
        const ssize_t got = ::recv( client_, in.data(), in.size(), 0 );
        if( got < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) ) {
            return true;
        }
        if( got <= 0 ) {
            end_transfers();
            close_socket( client_ );
            return false;
        }
        if( !consume( { in.data(), static_cast<std::size_t>( got ) } ) || client_ != fd ) {
            return false;
        }
    }
}

bool Bridge::consume( std::string_view bytes ) noexcept
{
    const int fd = client_;
    while( !bytes.empty() && client_ == fd ) {
        if( push_left_ != 0 ) {
            // File bytes of a push, straight to the file.
            const std::size_t take =
                static_cast<std::size_t>( std::min<std::uint64_t>( push_left_, bytes.size() ) );
            if( push_ >= 0 && !write_file( push_, bytes.substr( 0, take ) ) ) {
                note( Level::error, "push: could not write the file (is the storage full?)" );
                abandon_push(); // the rest of its bytes are still coming: skipped
            }
            push_left_ -= take;
            bytes.remove_prefix( take );
            if( push_left_ == 0 ) {
                finish_push();
            }
            continue;
        }
        const auto newline = bytes.find( '\n' );
        const std::string_view piece = bytes.substr( 0, newline );
        if( !cutting_ ) {
            const std::size_t room = command_.size() - command_size_;
            const std::size_t take = std::min( room, piece.size() );
            std::copy_n( piece.begin(), take,
                         command_.begin() + static_cast<std::ptrdiff_t>( command_size_ ) );
            command_size_ += take;
            if( take < piece.size() ) {
                // Longer than a command may be: run what fits, drop the rest.
                if( !run_line( { command_.data(), command_size_ } ) ) {
                    return false;
                }
                command_size_ = 0;
                cutting_ = true;
            }
        }
        if( newline == std::string_view::npos ) {
            return true; // the rest of this line comes later
        }
        bytes.remove_prefix( newline + 1 );
        if( cutting_ ) {
            cutting_ = false; // the end of a line that was too long: dropped
            command_size_ = 0;
            continue;
        }
        std::string_view line( command_.data(), command_size_ );
        if( !line.empty() && line.back() == '\r' ) {
            line.remove_suffix( 1 );
        }
        command_size_ = 0;
        if( !run_line( line ) ) {
            return false;
        }
    }
    return client_ == fd;
}

bool Bridge::run_line( std::string_view line ) noexcept
{
    const int fd = client_;
    if( line.substr( 0, protocol::push.size() ) == protocol::push ) {
        start_push( line.substr( protocol::push.size() ) );
        return client_ == fd;
    }
    if( handlers_.command != nullptr ) {
        handlers_.command( handlers_.context, line );
    }
    if( client_ != fd ) {
        return false; // the command closed the bridge
    }
    done(); // after the file, if this command started one or one is being sent
    return true;
}

void Bridge::start_push( std::string_view request ) noexcept
{
    // "<size> <name>"
    const auto space = request.find( ' ' );
    std::uint64_t size = 0;
    const std::string_view name =
        space == std::string_view::npos ? std::string_view{} : request.substr( space + 1 );
    if( space == std::string_view::npos || !parse_size( request.substr( 0, space ), size ) ) {
        note( Level::error, "push: the client sent a malformed push" );
        done();
        return;
    }
    push_size_ = size;
    push_left_ = size;
    push_ = -1;
    const std::size_t length = std::min( name.size(), push_name_.size() - 1 );
    std::copy_n( name.begin(), length, push_name_.begin() );
    push_name_[length] = '\0';
    if( push_directory_[0] == '\0' ) {
        note( Level::error, "push: this app takes no files" );
    } else if( !protocol::good_name( name ) ) {
        note( Level::error, "push: not a plain file name (no '/', not . or ..)" );
    } else {
        std::array<char, 400> part{};
        const int written = std::snprintf( part.data(), part.size(), "%s/%s.part",
                                           push_directory_.data(), push_name_.data() );
        push_ = written > 0 && static_cast<std::size_t>( written ) < part.size()
                    ? ::open( part.data(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600 )
                    : -1;
        if( push_ < 0 ) {
            note( Level::error, "push: could not create the file" );
        }
    }
    // A refused push still has its bytes on the way: they are skipped
    // (push_ stays -1) and the command ends after them.
    if( size == 0 ) {
        finish_push();
    }
}

void Bridge::finish_push() noexcept
{
    push_left_ = 0;
    if( push_ >= 0 ) {
        const bool closed = ::close( push_ ) == 0;
        push_ = -1;
        std::array<char, 400> part{};
        std::array<char, 400> final{};
        std::snprintf( part.data(), part.size(), "%s/%s.part", push_directory_.data(),
                       push_name_.data() );
        std::snprintf( final.data(), final.size(), "%s/%s", push_directory_.data(),
                       push_name_.data() );
        if( closed && ::rename( part.data(), final.data() ) == 0 ) {
            std::array<char, 512> line{};
            const int length =
                std::snprintf( line.data(), line.size(), "push: %s, %llu bytes", final.data(),
                               static_cast<unsigned long long>( push_size_ ) );
            note( Level::info, { line.data(), static_cast<std::size_t>( std::max( length, 0 ) ) } );
        } else {
            ::unlink( part.data() );
            note( Level::error, "push: could not finish the file" );
        }
    }
    done();
}

void Bridge::done() noexcept
{
    // Held back while a file is sent, while a command's work goes on
    // (set_busy), or while earlier ones still wait, so every "." keeps its
    // command's place.
    if( busy_ || file_ >= 0 || dones_after_file_ != 0 ) {
        if( dones_after_file_ != UINT16_MAX ) {
            ++dones_after_file_;
        }
        return;
    }
    queue( protocol::done, {} );
}

void Bridge::set_busy( bool busy ) noexcept
{
    if( busy == busy_ ) {
        return;
    }
    busy_ = busy;
    if( !busy && client_ >= 0 && file_ < 0 && dones_after_file_ != 0 ) {
        release_dones();
        flush();
    }
}

void Bridge::release_dones() noexcept
{
    if( busy_ ) {
        return; // the work goes on: its marks stay behind it
    }
    while( dones_after_file_ != 0 && append( protocol::done, {} ) ) {
        --dones_after_file_;
    }
}

void Bridge::note( Level level, std::string_view line ) noexcept
{
    if( handlers_.note != nullptr ) {
        handlers_.note( handlers_.context, level,
                        line ); // the Console, and back out through its mirror
    } else {
        send( level, line );
    }
}

bool Bridge::send_file( int fd, std::string_view name, std::uint64_t size ) noexcept
{
    if( client_ < 0 || file_ >= 0 || fd < 0 || !protocol::good_name( name ) ) {
        return false;
    }
    std::array<char, 160> header{};
    const int length = size == unknown_size
                           ? std::snprintf( header.data(), header.size(), "? %.*s",
                                            static_cast<int>( name.size() ), name.data() )
                           : std::snprintf( header.data(), header.size(), "%llu %.*s",
                                            static_cast<unsigned long long>( size ),
                                            static_cast<int>( name.size() ), name.data() );
    if( length <= 0 ||
        !queue( protocol::file, { header.data(), static_cast<std::size_t>( length ) } ) ) {
        return false;
    }
    file_ = fd;
    file_left_ = size;
    if( !writing_ ) {
        writing_ = true;
        watch_client();
    }
    flush();
    return true;
}

void Bridge::send( Level level, std::string_view line ) noexcept
{
    if( client_ < 0 ) {
        return;
    }
    const char type = level == Level::error     ? protocol::error
                      : level == Level::warning ? protocol::warning
                                                : protocol::info;
    queue( type, line );
}

bool Bridge::queue( char type, std::string_view text ) noexcept
{
    if( !append( type, text ) ) {
        return false;
    }
    flush();
    return true;
}

bool Bridge::append( char type, std::string_view text ) noexcept
{
    if( client_ < 0 ) {
        return false;
    }
    if( unreported_ != 0 && type != '\0' ) {
        // Say what was lost, first, once it fits.
        std::array<char, 64> notice{};
        const int length = std::snprintf( notice.data(), notice.size(),
                                          "bridge: %llu lines dropped (the client read too slowly)",
                                          static_cast<unsigned long long>( unreported_ ) );
        const std::size_t need = static_cast<std::size_t>( std::max( length, 0 ) ) + 3;
        if( output_.size() - output_size_ < need ) {
            ++dropped_;
            ++unreported_;
            return false;
        }
        const std::uint64_t lost = unreported_;
        unreported_ = 0;
        if( !append( protocol::warning, { notice.data(), need - 3 } ) ) {
            unreported_ += lost;
        }
    }
    text = text.substr( 0, line_bytes - 3 );
    const bool typed = type != '\0';
    const bool spaced = typed && type != protocol::done;
    const std::size_t need = ( typed ? 1U : 0U ) + ( spaced ? 1U : 0U ) + text.size() + 1;
    if( output_.size() - output_size_ < need ) {
        ++dropped_;
        ++unreported_;
        return false;
    }
    auto put = [this]( char c ) {
        output_[( output_head_ + output_size_ ) % output_.size()] = c;
        ++output_size_;
    };
    if( typed ) {
        put( type );
    }
    if( spaced ) {
        put( ' ' );
    }
    for( const char c : text ) {
        put( c == '\n' || c == '\r' ? ' ' : c ); // one line stays one line
    }
    put( '\n' );
    return true;
}

void Bridge::append_bytes( std::string_view bytes ) noexcept
{
    for( const char c : bytes ) {
        output_[( output_head_ + output_size_ ) % output_.size()] = c;
        ++output_size_;
    }
}

bool Bridge::pump_file() noexcept
{
    // "d <n>\n" and n bytes, only when they fit whole.
    constexpr std::size_t header_room = 16;
    if( output_.size() - output_size_ < protocol::chunk_bytes + header_room ) {
        return false;
    }
    std::array<char, protocol::chunk_bytes> chunk{};
    const std::size_t want =
        static_cast<std::size_t>( std::min<std::uint64_t>( file_left_, chunk.size() ) );
    ssize_t got = 0;
    if( want != 0 ) {
        do {
            got = ::read( file_, chunk.data(), want );
        } while( got < 0 && errno == EINTR );
    }
    const bool unknown = file_left_ == unknown_size;
    if( want != 0 && got > 0 ) {
        std::array<char, header_room> header{};
        const int length =
            std::snprintf( header.data(), header.size(), "%c %zd\n", protocol::data, got );
        append_bytes( { header.data(), static_cast<std::size_t>( length ) } );
        append_bytes( { chunk.data(), static_cast<std::size_t>( got ) } );
        if( !unknown ) {
            file_left_ -= static_cast<std::uint64_t>( got );
        }
    }
    if( file_left_ != 0 && got > 0 ) {
        return true;
    }
    // The end: all of it, or the file ended early (it shrank) or failed. A
    // file of unknown size ends when it reads nothing more.
    if( file_left_ != 0 && !unknown ) {
        append( protocol::error, "pull: the file ended before its size; the copy is short" );
    }
    ::close( file_ );
    file_ = -1;
    file_left_ = 0;
    release_dones(); // the file's own end mark, then any held back behind it
    return true;
}

void Bridge::flush() noexcept
{
    int chunks = 0;
    while( client_ >= 0 ) {
        while( output_size_ != 0 ) {
            const std::size_t contiguous = std::min( output_size_, output_.size() - output_head_ );
            const ssize_t wrote = ::send( client_, output_.data() + output_head_, contiguous,
                                          MSG_NOSIGNAL | MSG_DONTWAIT );
            if( wrote < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
                if( !writing_ ) {
                    writing_ = true;
                    watch_client();
                }
                return;
            }
            if( wrote < 0 && errno == EINTR ) {
                continue;
            }
            if( wrote <= 0 ) {
                end_transfers();
                close_socket( client_ );
                output_size_ = 0;
                return;
            }
            output_head_ = ( output_head_ + static_cast<std::size_t>( wrote ) ) % output_.size();
            output_size_ -= static_cast<std::size_t>( wrote );
        }
        output_head_ = 0;
        // End marks that did not fit when their file ended.
        if( file_ < 0 && !busy_ && dones_after_file_ != 0 ) {
            release_dones();
            continue;
        }
        // The buffer is empty: the next chunk of a file being sent, a few at a
        // time, then the socket is waited for again (writing stays watched).
        if( file_ < 0 || chunks == chunks_per_turn || !pump_file() ) {
            break;
        }
        ++chunks;
    }
    if( client_ >= 0 && output_size_ == 0 && file_ < 0 && writing_ ) {
        writing_ = false;
        watch_client();
    }
}

void Bridge::watch_client() noexcept
{
    if( client_ >= 0 && handlers_.watch != nullptr ) {
        handlers_.watch( handlers_.context, client_, true, writing_ );
    }
}

} // namespace sq::console
