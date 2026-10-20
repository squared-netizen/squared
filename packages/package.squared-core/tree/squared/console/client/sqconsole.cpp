// sqconsole: the app's Console, in a terminal on the same device.
//
//   sqconsole [--conf FILE] [--into DIR]                 interactive: the log as it
//                                                        is written, and every line
//                                                        you type is a command
//   sqconsole [--conf FILE] [--into DIR] --run CMD ...   run each command, print its
//                                                        output, exit
//             [--wait SECONDS]                           how long to wait for the app
//                                                        to start (30; 0: not at all)
//             [--until-closed]                           then wait for the app to close
//                                                        the connection: `--run quit
//                                                        --until-closed` returns once
//                                                        the app has gone
//
// FILE (default build/dev/bridge.conf, which a CONSOLE build writes) holds
// `port=`, `secret=` and `package=` lines. The app's end is
// sq::console::Bridge; the wire format is console/src/bridge_protocol.hpp.
//
// Files: the app's `pull <path>` sends one here, saved in DIR (default
// build/pulled); `push <file> [name]`, typed here, sends one of yours to the
// app's files/.
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
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

namespace protocol = sq::console::bridge_protocol;

struct Settings
{
        std::string conf = "build/dev/bridge.conf";
        std::string into = "build/pulled";
        std::vector<std::string> run;
        int wait_seconds = 30;     // --run only: how long to wait for the app to start
        bool until_closed = false; // --run only: success is the app closing the connection
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

void note( const std::string &text )
{
    std::fprintf( stderr, "%s%s%s\n", colour ? "\x1b[2m" : "", text.c_str(),
                  colour ? "\x1b[0m" : "" );
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

bool send_all( int fd, std::string_view bytes )
{
    while( !bytes.empty() ) {
        const ssize_t sent = ::send( fd, bytes.data(), bytes.size(), MSG_NOSIGNAL );
        if( sent < 0 && errno == EINTR ) {
            continue;
        }
        if( sent <= 0 ) {
            return false;
        }
        bytes.remove_prefix( static_cast<std::size_t>( sent ) );
    }
    return true;
}

bool send_line( int fd, std::string_view line )
{
    std::string out( line );
    out += '\n';
    return send_all( fd, out );
}

/// The words of "push <file> [name]", or false.
bool parse_push( std::string_view line, std::string &file, std::string &name )
{
    if( line.substr( 0, protocol::push.size() ) != protocol::push ) {
        return false;
    }
    std::string_view rest = line.substr( protocol::push.size() );
    while( !rest.empty() && rest.front() == ' ' ) {
        rest.remove_prefix( 1 );
    }
    const auto space = rest.find( ' ' );
    file = rest.substr( 0, space );
    name.clear();
    if( space != std::string_view::npos ) {
        std::string_view given = rest.substr( space + 1 );
        while( !given.empty() && given.front() == ' ' ) {
            given.remove_prefix( 1 );
        }
        name = given;
    }
    if( name.empty() ) {
        const auto slash = file.rfind( '/' );
        name = slash == std::string::npos ? file : file.substr( slash + 1 );
    }
    return true;
}

/// `push <file> [name]`: the header, then the file's bytes. False (and said
/// why) when nothing was sent, so no end mark will come for it.
bool push_file( int fd, std::string_view line )
{
    std::string file;
    std::string name;
    parse_push( line, file, name );
    if( file.empty() ) {
        note( "sqconsole: push <file> [name]: a file of yours to the app's files/" );
        return false;
    }
    if( !protocol::good_name( name ) ) {
        note( "sqconsole: push: the name must be a plain file name (no '/')" );
        return false;
    }
    const int local = ::open( file.c_str(), O_RDONLY | O_CLOEXEC );
    struct stat status = {};
    if( local < 0 || ::fstat( local, &status ) != 0 || !S_ISREG( status.st_mode ) ) {
        note( "sqconsole: push: " + file + ": " +
              ( local < 0 ? std::string( std::strerror( errno ) ) : "not a regular file" ) );
        if( local >= 0 ) {
            ::close( local );
        }
        return false;
    }
    const auto size = static_cast<std::uint64_t>( status.st_size );
    bool good = send_line( fd, "push " + std::to_string( size ) + " " + name );
    std::array<char, protocol::chunk_bytes> chunk{};
    std::uint64_t left = size;
    while( good && left != 0 ) {
        const ssize_t got = ::read( local, chunk.data(), chunk.size() );
        if( got <= 0 ) {
            // The file shrank: pad, so the app gets the size it was promised,
            // and say so (the app's copy ends in zeros).
            std::fill( chunk.begin(), chunk.end(), '\0' );
            note( "sqconsole: push: " + file + " shrank while it was sent; the copy is padded" );
            while( good && left != 0 ) {
                const std::size_t n =
                    left < chunk.size() ? static_cast<std::size_t>( left ) : chunk.size();
                good = send_all( fd, { chunk.data(), n } );
                left -= n;
            }
            break;
        }
        const std::size_t n = static_cast<std::uint64_t>( got ) < left
                                  ? static_cast<std::size_t>( got )
                                  : static_cast<std::size_t>( left );
        good = send_all( fd, { chunk.data(), n } );
        left -= n;
    }
    ::close( local );
    return good;
}

void make_directories( const std::string &path )
{
    for( std::size_t at = 1; at <= path.size(); ++at ) {
        if( at == path.size() || path[at] == '/' ) {
            ::mkdir( path.substr( 0, at ).c_str(), 0700 );
        }
    }
}

/// Everything from the app: lines, and the files it pulls into `into`.
class Session
{
    public:
        explicit Session( std::string into )
            : into_( std::move( into ) )
        {}
        Session( const Session & ) = delete;
        Session &operator=( const Session & ) = delete;
        Session( Session && ) = delete;
        Session &operator=( Session && ) = delete;
        ~Session() { abandon(); }

        /// Read what is waiting; `line( type, text )` for each line, 'o' for "ok".
        /// False when the app closed the connection.
        template<typename Line>
        bool read( int fd, Line line )
        {
            const ssize_t got = ::recv( fd, buffer_.data() + held_, buffer_.size() - held_, 0 );
            if( got <= 0 ) {
                return false;
            }
            held_ += static_cast<std::size_t>( got );
            std::size_t start = 0;
            while( start < held_ ) {
                if( data_left_ != 0 ) {
                    const std::size_t n = std::min<std::size_t>( data_left_, held_ - start );
                    write_data( { buffer_.data() + start, n } );
                    data_left_ -= n;
                    start += n;
                    continue;
                }
                const std::string_view rest( buffer_.data() + start, held_ - start );
                const auto end = rest.find( '\n' );
                if( end == std::string_view::npos ) {
                    break;
                }
                const std::string_view text = rest.substr( 0, end );
                start += end + 1;
                handle( text, line );
            }
            if( start == 0 && held_ == buffer_.size() ) {
                held_ = 0; // a line longer than any the app sends: dropped
            } else {
                std::memmove( buffer_.data(), buffer_.data() + start, held_ - start );
                held_ -= start;
            }
            return true;
        }

        void reset()
        {
            held_ = 0;
            data_left_ = 0;
            abandon();
        }

    private:
        template<typename Line>
        void handle( std::string_view text, Line &line )
        {
            if( text == protocol::ok ) {
                line( 'o', text );
            } else if( text.size() == 1 && text[0] == protocol::done ) {
                finish();
                line( protocol::done, {} );
            } else if( text.size() >= 2 && text[0] == protocol::file && text[1] == ' ' ) {
                begin( text.substr( 2 ) );
            } else if( text.size() >= 2 && text[0] == protocol::data && text[1] == ' ' ) {
                std::uint64_t n = 0;
                const std::string_view number = text.substr( 2 );
                std::from_chars( number.data(), number.data() + number.size(), n );
                data_left_ = n;
            } else if( text.size() >= 2 && text[1] == ' ' ) {
                line( text[0], text.substr( 2 ) );
            } else {
                line( protocol::info, text );
            }
        }

        /// "p <size|?> <name>": a file is coming.
        void begin( std::string_view header )
        {
            abandon();
            const auto space = header.find( ' ' );
            const std::string_view size = header.substr( 0, space );
            const std::string_view name =
                space == std::string_view::npos ? std::string_view{} : header.substr( space + 1 );
            expected_ = -1;
            if( size != "?" ) {
                std::uint64_t n = 0;
                std::from_chars( size.data(), size.data() + size.size(), n );
                expected_ = static_cast<long long>( n );
            }
            received_ = 0;
            if( !protocol::good_name( name ) ) {
                note( "sqconsole: pull: the app sent an unusable name; not saved" );
                return;
            }
            make_directories( into_ );
            final_ = into_ + "/" + std::string( name );
            part_ = final_ + ".part";
            file_ = ::open( part_.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644 );
            if( file_ < 0 ) {
                note( "sqconsole: pull: cannot write " + part_ + ": " + std::strerror( errno ) );
            }
        }

        void write_data( std::string_view bytes )
        {
            received_ += bytes.size();
            while( file_ >= 0 && !bytes.empty() ) {
                const ssize_t wrote = ::write( file_, bytes.data(), bytes.size() );
                if( wrote <= 0 ) {
                    note( "sqconsole: pull: writing " + part_ + " failed" );
                    abandon();
                    return;
                }
                bytes.remove_prefix( static_cast<std::size_t>( wrote ) );
            }
        }

        /// The end mark: a file in progress is complete now.
        void finish()
        {
            if( file_ < 0 ) {
                return;
            }
            ::close( file_ );
            file_ = -1;
            const bool whole =
                expected_ < 0 || received_ == static_cast<std::uint64_t>( expected_ );
            if( whole && ::rename( part_.c_str(), final_.c_str() ) == 0 ) {
                note( "sqconsole: saved " + final_ + " (" + std::to_string( received_ ) +
                      " bytes)" );
            } else {
                note( "sqconsole: pull: " + final_ + " came short (" + std::to_string( received_ ) +
                      " bytes); kept as " + part_ );
            }
        }

        void abandon()
        {
            if( file_ >= 0 ) {
                ::close( file_ );
                file_ = -1;
                ::unlink( part_.c_str() );
            }
        }

        std::array<char, 16384> buffer_{};
        std::size_t held_ = 0;
        std::uint64_t data_left_ = 0;
        std::string into_;
        std::string part_;
        std::string final_;
        long long expected_ = -1;
        std::uint64_t received_ = 0;
        int file_ = -1;
};

int usage()
{
    std::fprintf( stderr, "usage: sqconsole [--conf FILE] [--into DIR] [--run COMMAND]... "
                          "[--wait SECONDS] [--until-closed]\n" );
    return 2;
}

/// --run: connect (waiting up to wait_seconds), run each command, print its
/// output, and exit 0; 1 if the app could not be reached. With until_closed,
/// keep printing until the app closes the connection, and exit 0 when it does.
int run_commands( const Bridge &bridge, const Settings &settings )
{
    constexpr int answer_seconds = 30; // the app is running; it answers or is stuck
    const int announce = listen_for_announce( bridge.port );
    int fd = connect_to( bridge, true );
    if( fd < 0 && announce >= 0 && settings.wait_seconds > 0 ) {
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
    Session session( settings.into );
    std::size_t expected = 0;
    std::size_t finished = 0;
    bool greeted = false;
    bool failed = false;
    bool closed = false;
    while( !failed && !closed && ( !greeted || finished < expected || settings.until_closed ) ) {
        pollfd ready{ fd, POLLIN, 0 };
        if( ::poll( &ready, 1, answer_seconds * 1000 ) <= 0 ) {
            // Connected, but nothing runs: Android can freeze an app left in
            // the background, and its socket still accepts.
            std::fprintf( stderr,
                          "sqconsole: no answer for %d s; an app in the background can be "
                          "frozen by Android: bring it to the front, or keep it on screen "
                          "beside Termux\n",
                          answer_seconds );
            failed = true;
            break;
        }
        const bool open = session.read( fd, [&]( char type, std::string_view text ) {
            if( type == 'o' ) {
                greeted = true;
                for( const std::string &command : settings.run ) {
                    std::string file;
                    std::string name;
                    if( parse_push( command, file, name ) ) {
                        if( push_file( fd, command ) ) {
                            ++expected;
                        }
                    } else if( send_line( fd, command ) ) {
                        ++expected;
                    } else {
                        failed = true;
                    }
                }
            } else if( type == protocol::done ) {
                ++finished;
            } else {
                print_line( type, text );
            }
        } );
        if( !open && greeted && settings.until_closed ) {
            closed = true; // what was asked for
        } else if( !open ) {
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
int interact( const Bridge &bridge, const Settings &settings )
{
    const int announce = listen_for_announce( bridge.port );
    if( announce < 0 ) {
        note( "sqconsole: another sqconsole is waiting on this app; this one connects only now, "
              "not when the app restarts" );
    }
    Session session( settings.into );
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
                } else if( line.substr( 0, protocol::push.size() ) == protocol::push ||
                           line == "push" ) {
                    static_cast<void>( push_file( fd, line ) );
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
            const bool open = session.read( fd, [&]( char type, std::string_view text ) {
                if( type == 'o' ) {
                    greeted_now = true;
                } else if( type != protocol::done ) {
                    print_line( type, text );
                }
            } );
            if( greeted_now ) {
                note( "sqconsole: connected to " +
                      ( bridge.package.empty() ? std::string( "the app" ) : bridge.package ) );
            }
            if( !open ) {
                ::close( fd );
                fd = -1;
                session.reset();
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
        } else if( argument == "--into" && i + 1 < argc ) {
            settings.into = argv[++i];
        } else if( argument == "--run" && i + 1 < argc ) {
            settings.run.emplace_back( argv[++i] );
        } else if( argument == "--wait" && i + 1 < argc ) {
            settings.wait_seconds = std::atoi( argv[++i] );
        } else if( argument == "--until-closed" ) {
            settings.until_closed = true;
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
    return settings.run.empty() ? interact( bridge, settings ) : run_commands( bridge, settings );
}
