#include <squared/console/bridge.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace sq::console
{

namespace
{

namespace protocol = bridge_protocol;

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

void Bridge::close() noexcept
{
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
    // Bytes after the hello are the first commands.
    const std::string_view rest = held.substr( end + 1 );
    const int fd = pending_;
    take_over();
    command_size_ = std::min( rest.size(), command_.size() );
    std::copy_n( rest.begin(), command_size_, command_.begin() );
    if( handlers_.connected != nullptr ) {
        handlers_.connected( handlers_.context, replay );
    }
    if( command_size_ != 0 && client_ == fd ) {
        // Run what came with the hello, as if it had just arrived.
        const std::size_t held_bytes = command_size_;
        command_size_ = 0;
        std::array<char, command_bytes> copy{};
        std::copy_n( command_.begin(), held_bytes, copy.begin() );
        for( std::size_t start = 0; start < held_bytes; ) {
            const std::string_view chunk( copy.data() + start, held_bytes - start );
            const auto newline = chunk.find( '\n' );
            if( newline == std::string_view::npos ) {
                std::copy( chunk.begin(), chunk.end(), command_.begin() );
                command_size_ = chunk.size();
                break;
            }
            std::string_view command = chunk.substr( 0, newline );
            if( !command.empty() && command.back() == '\r' ) {
                command.remove_suffix( 1 );
            }
            if( handlers_.command != nullptr ) {
                handlers_.command( handlers_.context, command );
            }
            queue( protocol::done, {} );
            start += newline + 1;
        }
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
    for( ;; ) {
        const ssize_t got =
            ::recv( client_, command_.data() + command_size_, command_.size() - command_size_, 0 );
        if( got < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) ) {
            return true;
        }
        if( got <= 0 ) {
            close_socket( client_ );
            return false;
        }
        command_size_ += static_cast<std::size_t>( got );
        std::size_t start = 0;
        for( ;; ) {
            const std::string_view held( command_.data() + start, command_size_ - start );
            const auto end = held.find( '\n' );
            if( end == std::string_view::npos ) {
                break;
            }
            std::string_view command = held.substr( 0, end );
            if( !command.empty() && command.back() == '\r' ) {
                command.remove_suffix( 1 );
            }
            start += end + 1;
            if( cutting_ ) {
                cutting_ = false; // the end of a line that was too long: dropped
                continue;
            }
            if( handlers_.command != nullptr ) {
                handlers_.command( handlers_.context, command );
            }
            if( client_ != fd ) {
                return false; // the command closed the bridge
            }
            queue( protocol::done, {} );
        }
        if( start == 0 && command_size_ == command_.size() ) {
            // A command longer than the buffer: run what fits, drop the rest.
            if( !cutting_ && handlers_.command != nullptr ) {
                handlers_.command( handlers_.context, { command_.data(), command_size_ } );
                if( client_ != fd ) {
                    return false;
                }
                queue( protocol::done, {} );
            }
            cutting_ = true;
            command_size_ = 0;
        } else {
            std::memmove( command_.data(), command_.data() + start, command_size_ - start );
            command_size_ -= start;
        }
    }
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
        if( !queue( protocol::warning, { notice.data(), need - 3 } ) ) {
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
    flush();
    return true;
}

void Bridge::flush() noexcept
{
    while( client_ >= 0 && output_size_ != 0 ) {
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
            close_socket( client_ );
            output_size_ = 0;
            return;
        }
        output_head_ = ( output_head_ + static_cast<std::size_t>( wrote ) ) % output_.size();
        output_size_ -= static_cast<std::size_t>( wrote );
    }
    if( output_size_ == 0 ) {
        output_head_ = 0;
        if( writing_ ) {
            writing_ = false;
            watch_client();
        }
    }
}

void Bridge::watch_client() noexcept
{
    if( client_ >= 0 && handlers_.watch != nullptr ) {
        handlers_.watch( handlers_.context, client_, true, writing_ );
    }
}

} // namespace sq::console
