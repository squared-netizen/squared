// sqconsole: the app's Console, in a terminal on the same device.
//
//   sqconsole [--conf FILE]                 interactive: the log as it is written,
//                                           and every line you type is a command
//   sqconsole [--conf FILE] --run CMD ...   run each command, print its output, exit
//
// FILE (default build/dev/bridge.conf, which a CONSOLE build writes) holds
// `port=`, `secret=` and `package=` lines. The app's end is
// sq::console::Bridge; the wire format is console/src/bridge_protocol.hpp.
//
// It waits for the app without asking again and again: it listens for the
// one datagram the app sends when its bridge opens, and connects then. So it
// can be started before the app, and it reconnects when the app restarts.
// Ctrl-D or Ctrl-C ends it.
//
// A tool, not part of an app: built for the host (Termux, or a desktop) by
// `make console` in a project, and by the console module's tests.

#include <squared/console/line_reader.hpp>
#include <squared/console/src/bridge_protocol.hpp>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace
{

namespace protocol = sq::console::bridge_protocol;

struct Settings
{
        std::string conf = "build/dev/bridge.conf";
        std::vector<std::string> run;
        int wait_seconds = 30; // --run only: how long to wait for the app
};

struct Bridge
{
        std::uint16_t port = 0;
        std::string secret;
        std::string package;
};

bool colour = false;

void print_line( char type, std::string_view text )
{
    const char *start = "";
    const char *end = "";
    if( colour && type == protocol::error ) {
        start = "\x1b[31m";
        end = "\x1b[0m";
    } else if( colour && type == protocol::warning ) {
        start = "\x1b[33m";
        end = "\x1b[0m";
    }
    std::printf( "%s%.*s%s\n", start, static_cast<int>( text.size() ), text.data(), end );
    std::fflush( stdout );
}

void note( const char *text )
{
    std::fprintf( stderr, "%s%s%s\n", colour ? "\x1b[2m" : "", text, colour ? "\x1b[0m" : "" );
}

bool read_conf( const std::string &path, Bridge &out )
{
    const int fd = ::open( path.c_str(), O_RDONLY | O_CLOEXEC );
    if( fd < 0 ) {
        return false;
    }
    std::array<char, 256> buffer{};
    sq::console::for_each_line( fd, buffer, [&]( std::string_view text ) {
        if( !text.empty() && text.back() == '\r' ) {
            text.remove_suffix( 1 );
        }
        const auto equals = text.find( '=' );
        if( equals == std::string_view::npos ) {
            return;
        }
        const std::string_view key = text.substr( 0, equals );
        const std::string_view value = text.substr( equals + 1 );
        if( key == "port" ) {
            out.port = static_cast<std::uint16_t>(
                std::strtoul( std::string( value ).c_str(), nullptr, 10 ) );
        } else if( key == "secret" ) {
            out.secret = value;
        } else if( key == "package" ) {
            out.package = value;
        }
    } );
    ::close( fd );
    return out.port != 0 && !out.secret.empty();
}

sockaddr_in loopback( std::uint16_t port )
{
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons( port );
    address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    return address;
}

/// The socket the app's announcement arrives on, or -1 (another sqconsole has it).
int listen_for_announce( std::uint16_t port )
{
    const int fd = ::socket( AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0 );
    if( fd < 0 ) {
        return -1;
    }
    const sockaddr_in address = loopback( port );
    if( ::bind( fd, reinterpret_cast<const sockaddr *>( &address ), sizeof( address ) ) != 0 ) {
        ::close( fd );
        return -1;
    }
    return fd;
}

/// Connected and greeted, or -1 (the app is not listening).
int connect_to( const Bridge &bridge, bool quiet )
{
    const int fd = ::socket( AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0 );
    if( fd < 0 ) {
        return -1;
    }
    const sockaddr_in address = loopback( bridge.port );
    if( ::connect( fd, reinterpret_cast<const sockaddr *>( &address ), sizeof( address ) ) != 0 ) {
        ::close( fd );
        return -1;
    }
    std::string hello( protocol::hello );
    hello += bridge.secret;
    if( quiet ) {
        hello += protocol::quiet;
    }
    hello += '\n';
    if( ::send( fd, hello.data(), hello.size(), MSG_NOSIGNAL ) !=
        static_cast<ssize_t>( hello.size() ) ) {
        ::close( fd );
        return -1;
    }
    return fd;
}

bool send_line( int fd, std::string_view line )
{
    std::string out( line );
    out += '\n';
    return ::send( fd, out.data(), out.size(), MSG_NOSIGNAL ) == static_cast<ssize_t>( out.size() );
}

/// Lines from the app. Calls `line( type, text )`; "ok" arrives as type 'o'.
class Reader
{
    public:
        template<typename Line>
        bool read( int fd, Line line )
        {
            const ssize_t got = ::recv( fd, buffer_.data() + held_, buffer_.size() - held_, 0 );
            if( got <= 0 ) {
                return false;
            }
            held_ += static_cast<std::size_t>( got );
            std::size_t start = 0;
            for( ;; ) {
                const std::string_view rest( buffer_.data() + start, held_ - start );
                const auto end = rest.find( '\n' );
                if( end == std::string_view::npos ) {
                    break;
                }
                const std::string_view text = rest.substr( 0, end );
                start += end + 1;
                if( text == protocol::ok ) {
                    line( 'o', text );
                } else if( text.size() == 1 && text[0] == protocol::done ) {
                    line( protocol::done, {} );
                } else if( text.size() >= 2 && text[1] == ' ' ) {
                    line( text[0], text.substr( 2 ) );
                } else {
                    line( protocol::info, text );
                }
            }
            if( start == 0 && held_ == buffer_.size() ) {
                held_ = 0; // a line longer than any the app sends: dropped
            } else {
                std::memmove( buffer_.data(), buffer_.data() + start, held_ - start );
                held_ -= start;
            }
            return true;
        }

        void reset() { held_ = 0; }

    private:
        std::array<char, 8192> buffer_{};
        std::size_t held_ = 0;
};

int usage()
{
    std::fprintf( stderr, "usage: sqconsole [--conf FILE] [--run COMMAND]... [--wait SECONDS]\n" );
    return 2;
}

/// --run: connect (waiting up to wait_seconds), run each command, print its
/// output, and exit 0; 1 if the app could not be reached.
int run_commands( const Bridge &bridge, const Settings &settings )
{
    const int announce = listen_for_announce( bridge.port );
    int fd = connect_to( bridge, true );
    if( fd < 0 && announce >= 0 ) {
        std::fprintf( stderr, "sqconsole: %s is not running; waiting up to %d s for it to start\n",
                      bridge.package.empty() ? "the app" : bridge.package.c_str(),
                      settings.wait_seconds );
        pollfd waiting{ announce, POLLIN, 0 };
        if( ::poll( &waiting, 1, settings.wait_seconds * 1000 ) > 0 ) {
            fd = connect_to( bridge, true );
        }
    }
    if( announce >= 0 ) {
        ::close( announce );
    }
    if( fd < 0 ) {
        std::fprintf(
            stderr, "sqconsole: %s is not reachable on port %u; is it running, a CONSOLE build?\n",
            bridge.package.empty() ? "the app" : bridge.package.c_str(),
            static_cast<unsigned>( bridge.port ) );
        return 1;
    }
    Reader reader;
    std::size_t sent = 0;
    std::size_t finished = 0;
    bool greeted = false;
    bool failed = false;
    while( finished < settings.run.size() && !failed ) {
        pollfd ready{ fd, POLLIN, 0 };
        if( ::poll( &ready, 1, settings.wait_seconds * 1000 ) <= 0 ) {
            std::fprintf( stderr, "sqconsole: no answer for %d s\n", settings.wait_seconds );
            failed = true;
            break;
        }
        const bool open = reader.read( fd, [&]( char type, std::string_view text ) {
            if( type == 'o' ) {
                greeted = true;
                for( ; sent < settings.run.size(); ++sent ) {
                    if( !send_line( fd, settings.run[sent] ) ) {
                        failed = true;
                    }
                }
            } else if( type == protocol::done ) {
                ++finished;
            } else {
                print_line( type, text );
            }
        } );
        if( !open ) {
            std::fprintf( stderr, greeted ? "sqconsole: the app closed the connection\n"
                                          : "sqconsole: refused: is build/dev/bridge.conf from "
                                            "the build that is installed?\n" );
            failed = true;
        }
    }
    ::close( fd );
    return failed ? 1 : 0;
}

/// Interactive: until Ctrl-D, reconnecting whenever the app comes back.
int interact( const Bridge &bridge )
{
    const int announce = listen_for_announce( bridge.port );
    if( announce < 0 ) {
        note( "sqconsole: another sqconsole is waiting on this app; this one connects only now, "
              "not when the app restarts" );
    }
    Reader reader;
    int fd = connect_to( bridge, false );
    if( fd < 0 ) {
        note( "sqconsole: waiting for the app to start (Ctrl-D to stop)" );
    }
    std::array<char, 512> typed{};
    std::size_t typed_size = 0;
    for( ;; ) {
        std::array<pollfd, 3> watch{};
        nfds_t count = 0;
        watch[count++] = { STDIN_FILENO, POLLIN, 0 };
        const nfds_t socket_at = count;
        if( fd >= 0 ) {
            watch[count++] = { fd, POLLIN, 0 };
        }
        const nfds_t announce_at = count;
        if( announce >= 0 ) {
            watch[count++] = { announce, POLLIN, 0 };
        }
        if( ::poll( watch.data(), count, -1 ) < 0 ) {
            if( errno == EINTR ) {
                continue;
            }
            return 1;
        }
        if( ( watch[0].revents & ( POLLIN | POLLHUP ) ) != 0 ) {
            const ssize_t got =
                ::read( STDIN_FILENO, typed.data() + typed_size, typed.size() - typed_size );
            if( got <= 0 ) {
                break; // Ctrl-D
            }
            typed_size += static_cast<std::size_t>( got );
            std::size_t start = 0;
            for( ;; ) {
                const std::string_view rest( typed.data() + start, typed_size - start );
                const auto end = rest.find( '\n' );
                if( end == std::string_view::npos ) {
                    break;
                }
                const std::string_view line = rest.substr( 0, end );
                start += end + 1;
                if( fd < 0 ) {
                    note( "sqconsole: not connected; the app is not running" );
                } else if( !line.empty() ) {
                    static_cast<void>( send_line( fd, line ) );
                }
            }
            if( start == 0 && typed_size == typed.size() ) {
                typed_size = 0;
            } else {
                std::memmove( typed.data(), typed.data() + start, typed_size - start );
                typed_size -= start;
            }
        }
        if( fd >= 0 && socket_at < count &&
            ( watch[socket_at].revents & ( POLLIN | POLLHUP | POLLERR ) ) != 0 ) {
            bool greeted_now = false;
            const bool open = reader.read( fd, [&]( char type, std::string_view text ) {
                if( type == 'o' ) {
                    greeted_now = true;
                } else if( type != protocol::done ) {
                    print_line( type, text );
                }
            } );
            if( greeted_now ) {
                std::string text =
                    "sqconsole: connected to " +
                    ( bridge.package.empty() ? std::string( "the app" ) : bridge.package );
                note( text.c_str() );
            }
            if( !open ) {
                ::close( fd );
                fd = -1;
                reader.reset();
                note( "sqconsole: the app went away; waiting for it to start again" );
            }
        }
        if( announce >= 0 && announce_at < count && ( watch[announce_at].revents & POLLIN ) != 0 ) {
            std::array<char, 64> datagram{};
            static_cast<void>( ::recv( announce, datagram.data(), datagram.size(), 0 ) );
            if( fd < 0 ) {
                fd = connect_to( bridge, false );
            }
        }
    }
    if( fd >= 0 ) {
        ::close( fd );
    }
    if( announce >= 0 ) {
        ::close( announce );
    }
    return 0;
}

} // namespace

int main( int argc, char **argv )
{
    Settings settings;
    for( int i = 1; i < argc; ++i ) {
        const std::string_view argument( argv[i] );
        if( argument == "--conf" && i + 1 < argc ) {
            settings.conf = argv[++i];
        } else if( argument == "--run" && i + 1 < argc ) {
            settings.run.emplace_back( argv[++i] );
        } else if( argument == "--wait" && i + 1 < argc ) {
            settings.wait_seconds = std::atoi( argv[++i] );
        } else {
            return usage();
        }
    }
    colour = ::isatty( STDOUT_FILENO ) == 1;
    Bridge bridge;
    if( !read_conf( settings.conf, bridge ) ) {
        std::fprintf( stderr,
                      "sqconsole: no port and secret in %s; build the app with CONSOLE=app first\n",
                      settings.conf.c_str() );
        return 1;
    }
    return settings.run.empty() ? interact( bridge ) : run_commands( bridge, settings );
}
