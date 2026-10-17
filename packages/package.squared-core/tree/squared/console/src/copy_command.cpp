#include <squared/console/copy_command.hpp>

#include <cstddef>
#include <string_view>

namespace sq::console
{

namespace
{

void copy( Console &console, const Args &args, void *context ) noexcept
{
    const auto &writer = *static_cast<const ClipboardWriter *>( context );
    const bool errors_only = args.text( 1 ) == "errors";
    const long long asked = errors_only ? 0 : args.integer( 1, 0 );
    if( !errors_only && ( args.size() > 2 || ( args.size() == 2 && asked <= 0 ) ) ) {
        console.print( "copy: copy, copy errors, or copy <lines>", Level::error );
        return;
    }
    // Every line but the newest, which is this command's echo.
    const std::size_t held = console.lines().size();
    const std::size_t available = held > 0 ? held - 1 : 0;
    const std::size_t wanted = asked > 0 && static_cast<std::size_t>( asked ) < available
                                   ? static_cast<std::size_t>( asked )
                                   : available;
    if( writer.begin == nullptr || !writer.begin( writer.context ) ) {
        console.print( "copy: the clipboard is not available", Level::error );
        return;
    }
    std::size_t lines = 0;
    std::size_t bytes = 0;
    std::size_t index = 0;
    for( auto it = console.lines().last( wanted + 1 ); index < wanted; ++it, ++index ) {
        const Line line = *it;
        if( errors_only && line.level != Level::error ) {
            continue;
        }
        if( lines != 0 ) {
            writer.append( writer.context, "\n" );
            ++bytes;
        }
        writer.append( writer.context, line.text );
        bytes += line.text.size();
        ++lines;
    }
    if( !writer.end( writer.context ) ) {
        console.print( "copy: the clipboard refused the text", Level::error );
        return;
    }
    console.printf( Level::info, "copied %zu line%s, %zu bytes", lines, lines == 1 ? "" : "s",
                    bytes );
}

} // namespace

Commands::AddResult add_copy_command( Console &console, const ClipboardWriter &writer ) noexcept
{
    // The table keeps the context as void*; the command only reads through it.
    return console.add( "copy", "copy [errors | <lines>]: the scrollback to the clipboard", copy,
                        const_cast<ClipboardWriter *>( &writer ) );
}

} // namespace sq::console
