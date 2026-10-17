#include <squared/console/console.hpp>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace sq::console
{

namespace
{

void help( Console &console, const Args &args, void * ) noexcept
{
    const auto &all = console.commands().all();
    if( args.size() > 1 ) {
        const Command *command = console.commands().find( args.text( 1 ) );
        if( command == nullptr ) {
            console.printf( Level::error, "no command named %.*s",
                            static_cast<int>( args.text( 1 ).size() ), args.text( 1 ).data() );
            return;
        }
        console.printf( Level::info, "%s  %s", command->name, command->help );
        return;
    }
    for( const Command &command : all ) {
        console.printf( Level::info, "%-10s %s", command.name, command.help );
    }
}

void clear( Console &console, const Args &, void * ) noexcept
{
    console.lines().clear();
}

void close( Console &console, const Args &, void * ) noexcept
{
    console.hide();
}

} // namespace

Console::Console() noexcept
    : lines_( storage_ )
{
    static_cast<void>(
        commands_.add( "help", "help [command]: list the commands, or show one", help ) );
    static_cast<void>( commands_.add( "clear", "clear: empty the scrollback", clear ) );
    static_cast<void>( commands_.add( "close", "close: hide the console", close ) );
}

void Console::print( std::string_view text, Level level ) noexcept
{
    lines_.push( level, text );
}

void Console::printf( Level level, const char *format, ... ) noexcept
{
    std::va_list arguments;
    va_start( arguments, format );
    const int written = std::vsnprintf( format_.data(), format_.size(), format, arguments );
    va_end( arguments );
    if( written < 0 ) {
        return;
    }
    const auto length = std::min( static_cast<std::size_t>( written ), format_.size() - 1 );
    lines_.push( level, { format_.data(), length } );
}

void Console::run( std::string_view line ) noexcept
{
    printf( Level::info, "> %.*s", static_cast<int>( line.size() ), line.data() );
    if( !args_.parse( line ) ) {
        printf( Level::error, "too many words: at most %zu", max_words );
        return;
    }
    if( args_.size() == 0 ) {
        return;
    }
    const Command *command = commands_.find( args_.text( 0 ) );
    if( command == nullptr ) {
        printf( Level::error, "unknown command: %.*s (help lists them)",
                static_cast<int>( args_.text( 0 ).size() ), args_.text( 0 ).data() );
        return;
    }
    command->handler( *this, args_, command->context );
}

void Console::submit() noexcept
{
    // The command runs on a copy: the words are views into it, and a command
    // may edit the input line while it runs.
    const std::string_view typed = input_.text();
    std::memcpy( running_.data(), typed.data(), typed.size() );
    const std::string_view line( running_.data(), typed.size() );
    input_.remember();
    input_.clear();
    run( line );
}

std::size_t Console::complete() noexcept
{
    const std::string_view typed = input_.text();
    if( typed.find( ' ' ) != std::string_view::npos ) {
        return 0; // only the command's name is completed
    }
    std::size_t matches = 0;
    std::string_view common;
    for( const Command &command : commands_.all() ) {
        const std::string_view name = command.name;
        if( name.substr( 0, typed.size() ) != typed ) {
            continue;
        }
        if( matches++ == 0 ) {
            common = name;
        } else {
            std::size_t n = 0;
            while( n < common.size() && n < name.size() && common[n] == name[n] ) {
                ++n;
            }
            common = common.substr( 0, n );
        }
    }
    if( matches == 1 ) {
        input_.set( common );
        input_.insert( " " );
    } else if( matches > 1 ) {
        input_.set( common );
        // List them on one line, as many as fit the format buffer.
        std::size_t used = 0;
        for( const Command &command : commands_.all() ) {
            const std::string_view name = command.name;
            if( name.substr( 0, typed.size() ) != typed ||
                used + name.size() + 2 > format_.size() ) {
                continue;
            }
            std::memcpy( format_.data() + used, name.data(), name.size() );
            used += name.size();
            format_[used++] = ' ';
        }
        lines_.push( Level::info, { format_.data(), used > 0 ? used - 1 : 0 } );
    }
    return matches;
}

bool Console::key( app::Key key, app::KeyModifiers ) noexcept
{
    switch( key ) {
        case app::Key::left:
            input_.left();
            return true;
        case app::Key::right:
            input_.right();
            return true;
        case app::Key::up:
            input_.previous();
            return true;
        case app::Key::down:
            input_.next();
            return true;
        case app::Key::home:
            input_.home();
            return true;
        case app::Key::end:
            input_.end();
            return true;
        case app::Key::backspace:
            input_.erase_before();
            return true;
        case app::Key::delete_key:
            input_.erase_after();
            return true;
        case app::Key::enter:
            submit();
            return true;
        case app::Key::tab:
            static_cast<void>( complete() );
            return true;
        case app::Key::escape:
            input_.clear();
            return true;
        // A space arrives as typed text, through text(); taking the key as
        // well would type it twice.
        case app::Key::space:
        case app::Key::unknown:
            return false;
    }
    return false;
}

} // namespace sq::console
