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

constexpr const char *dump_usage =
    "dump <group>...: run groups of commands, one a frame; dump all, dump stop";
constexpr const char *dump_help =
    "dump [group...]: run groups of commands (sys, memory, ...); all; stop";

// Each group and its lines, as one line: "sys      mem, heap, threads".
void list_groups( Console &console ) noexcept
{
    const Groups &groups = console.groups();
    if( groups.names().empty() ) {
        console.print( "dump: no groups yet" );
        return;
    }
    std::array<char, format_bytes> joined{};
    for( std::size_t group = 0; group < groups.names().size(); ++group ) {
        std::size_t used = 0;
        for( const GroupLine &line : groups.lines() ) {
            if( line.group != group ) {
                continue;
            }
            const char *separator = used == 0 ? "" : ", ";
            const int wrote = std::snprintf( joined.data() + used, joined.size() - used, "%s%s",
                                             separator, line.line );
            const std::size_t grew = used + static_cast<std::size_t>( std::max( wrote, 0 ) );
            used = std::min( grew, joined.size() - 1 );
        }
        console.printf( Level::info, "  %-9s %s", groups.names()[group], joined.data() );
    }
}

void dump( Console &console, const Args &args, void * ) noexcept
{
    if( args.size() == 1 ) {
        console.print( dump_usage );
        list_groups( console );
        return;
    }
    if( args.size() == 2 && args.text( 1 ) == "stop" ) {
        const std::size_t left = console.stop_dump();
        console.printf( Level::info, "dump: stopped; %zu not run", left );
        return;
    }
    std::size_t queued = 0;
    std::size_t full = 0;
    for( std::size_t word = 1; word < args.size(); ++word ) {
        const std::string_view group = args.text( word );
        const Console::Queued result = console.queue_group( group );
        if( !result.found ) {
            console.printf( Level::error, "dump: no group named %.*s (dump lists them)",
                            static_cast<int>( group.size() ), group.data() );
        }
        queued += result.queued;
        full += result.full;
    }
    if( full != 0 ) {
        console.printf( Level::warning, "dump: the queue is full; %zu not queued", full );
    }
    if( queued != 0 ) {
        console.printf( Level::info, "dump: %zu queued, one a frame", queued );
    }
}

} // namespace

Console::Console() noexcept
    : lines_( storage_ )
{
    static_cast<void>(
        commands_.add( "help", "help [command]: list the commands, or show one", help ) );
    static_cast<void>( commands_.add( "clear", "clear: empty the scrollback", clear ) );
    static_cast<void>( commands_.add( "close", "close: hide the console", close ) );
    static_cast<void>( commands_.add( "dump", dump_help, dump ) );
}

Groups::AddResult Console::add_group( const char *group,
                                      std::initializer_list<const char *> lines ) noexcept
{
    for( const char *line : lines ) {
        const Groups::AddResult result = groups_.add( group, line );
        if( result != Groups::AddResult::added ) {
            return result;
        }
    }
    return Groups::AddResult::added;
}

Console::Queued Console::queue_group( std::string_view group ) noexcept
{
    Queued result;
    const bool all = group == "all";
    const std::optional<std::size_t> number = all ? std::nullopt : groups_.find( group );
    if( !all && !number ) {
        return result;
    }
    result.found = true;
    for( const GroupLine &line : groups_.lines() ) {
        if( !all && line.group != *number ) {
            continue;
        }
        bool waiting = false;
        for( std::size_t i = 0; i < queued_ && !waiting; ++i ) {
            waiting = std::string_view( queue_[( queue_head_ + i ) % queue_.size()] ) == line.line;
        }
        if( waiting ) {
            continue;
        }
        if( queued_ == queue_.size() ) {
            ++result.full;
            continue;
        }
        queue_[( queue_head_ + queued_ ) % queue_.size()] = line.line;
        ++queued_;
        ++result.queued;
    }
    return result;
}

std::size_t Console::stop_dump() noexcept
{
    const std::size_t left = queued_;
    queued_ = 0;
    queue_head_ = 0;
    dump_run_ = 0;
    return left;
}

void Console::update() noexcept
{
    if( queued_ == 0 ) {
        return;
    }
    const char *line = queue_[queue_head_];
    queue_head_ = ( queue_head_ + 1 ) % queue_.size();
    --queued_;
    ++dump_run_;
    run( line );
    if( queued_ == 0 && dump_run_ != 0 ) {
        printf( Level::info, "dump: done, %zu run", dump_run_ );
        dump_run_ = 0;
    }
}

void Console::print( std::string_view text, Level level ) noexcept
{
    lines_.push( level, text );
    if( mirror_ == nullptr ) {
        return;
    }
    // One call a line, as the scrollback splits them.
    while( !text.empty() ) {
        const auto end = text.find( '\n' );
        const auto line = text.substr( 0, end );
        mirror_( mirror_context_, level, line );
        text.remove_prefix( end == std::string_view::npos ? text.size() : end + 1 );
    }
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
    print( { format_.data(), length }, level );
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
