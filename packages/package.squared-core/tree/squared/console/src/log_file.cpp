#include <squared/console/line_reader.hpp>
#include <squared/console/log_file.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace sq::console
{

namespace
{

bool write_all( int fd, std::string_view bytes ) noexcept
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

void log_command( Console &console, const Args &args, void *context ) noexcept
{
    const auto *previous_path = static_cast<const char *>( context );
    if( args.text( 1 ) != "previous" ) {
        console.print( "log: log previous", Level::error );
        return;
    }
    if( !show_log( console, previous_path, "log: how the last run ended" ) ) {
        console.print( "log: no earlier run's log is kept" );
    }
}

} // namespace

bool save_log( const Console &console, const char *path, std::size_t count ) noexcept
{
    std::array<char, 512> temporary{};
    const int length = std::snprintf( temporary.data(), temporary.size(), "%s.tmp", path );
    if( length <= 0 || static_cast<std::size_t>( length ) >= temporary.size() - 8 ) {
        return false;
    }
    const int fd = ::open( temporary.data(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600 );
    if( fd < 0 ) {
        return false;
    }
    bool good = true;
    const Scrollback &lines = console.lines();
    for( auto at = lines.last( count ); good && at != lines.end(); ++at ) {
        good = write_all( fd, ( *at ).text ) && write_all( fd, "\n" );
    }
    good = ::close( fd ) == 0 && good;
    if( !good || ::rename( temporary.data(), path ) != 0 ) {
        ::unlink( temporary.data() );
        return false;
    }
    return true;
}

bool keep_previous_log( const char *path, const char *previous_path ) noexcept
{
    return ::rename( path, previous_path ) == 0;
}

bool show_log( Console &console, const char *path, std::string_view title ) noexcept
{
    const int fd = ::open( path, O_RDONLY | O_CLOEXEC );
    if( fd < 0 ) {
        return false;
    }
    console.print( title );
    std::array<char, 1024> buffer{};
    for_each_line( fd, buffer, [&]( std::string_view line ) { console.print( line ); } );
    ::close( fd );
    return true;
}

Commands::AddResult add_log_command( Console &console, const char *previous_path ) noexcept
{
    return console.add( "log", "log previous: how the last run ended (its last lines)", log_command,
                        const_cast<char *>( previous_path ) );
}

} // namespace sq::console
