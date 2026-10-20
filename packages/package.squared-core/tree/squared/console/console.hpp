#pragma once

#include <squared/app/src/key.hpp>
#include <squared/app/src/key_modifiers.hpp>
#include <squared/console/args.hpp>
#include <squared/console/commands.hpp>
#include <squared/console/input_line.hpp>
#include <squared/console/scrollback.hpp>

#include <array>
#include <cstddef>
#include <string_view>

namespace sq::console
{

/**
 * @brief Called with every line the Console prints, after it is in the
 * scrollback: the bridge to Termux uses it (bridge.hpp). Function pointer and
 * context, no allocation; it must not print into the same Console.
 */
using Mirror = void ( * )( void *context, Level level, std::string_view line ) noexcept;

/**
 * @brief Text in, text out: the escape hatch for when the rest is broken.
 *
 * This is the Console's state and behaviour, with no drawing: the scrollback,
 * the input line, the command table. The view that draws it comes separately.
 * Everything is inline and sized at compile time (about 68 KB by default,
 * nearly all of it the scrollback), so hold it in something long-lived, never
 * on the stack. Nothing it does allocates. One thread uses it.
 *
 * ```cpp
 * console.add( "spawn", "spawn <monster> <count>",
 *              []( sq::console::Console &c, const sq::console::Args &a, void * ) {
 *                  c.printf( sq::console::Level::info, "spawned %lld %.*s",
 *                            static_cast<long long>( a.integer( 2, 1 ) ),
 *                            static_cast<int>( a.text( 1 ).size() ), a.text( 1 ).data() );
 *              } );
 * console.run( "spawn goblin 3" );
 * ```
 */
class Console final
{
    public:
        /** @brief An empty, hidden console with its built-ins: `help`, `clear`, `close`. */
        Console() noexcept;
        ~Console() = default;
        Console( const Console & ) = delete;
        Console &operator=( const Console & ) = delete;
        Console( Console && ) = delete;
        Console &operator=( Console && ) = delete;

        /** @brief Register a command. See Commands::add for what can refuse. */
        Commands::AddResult add( const char *name, const char *help, Handler handler,
                                 void *context = nullptr ) noexcept
        {
            return commands_.add( name, help, handler, context );
        }

        /**
         * @brief Hand every printed line to `mirror` as well, the echo of a
         * command and its output included; nullptr stops it. One at a time.
         */
        void set_mirror( Mirror mirror, void *context ) noexcept
        {
            mirror_ = mirror;
            mirror_context_ = context;
        }

        /** @brief Add text to the scrollback, one line per `\n` part. */
        void print( std::string_view text, Level level = Level::info ) noexcept;

        /** @brief Format with printf rules into one fixed buffer (format_bytes), then print. */
        [[gnu::format( printf, 3, 4 )]] void printf( Level level, const char *format,
                                                     ... ) noexcept;

        /**
         * @brief Run a command line: echo it, split it, call its command.
         *
         * An unknown name, or too many words, prints an error line. The words
         * are views into `line`, which must stay put until this returns.
         */
        void run( std::string_view line ) noexcept;

        /** @brief Run the input line (Enter): remember it in the history, run it, clear it. */
        void submit() noexcept;

        /**
         * @brief Complete the command name being typed (Tab).
         * @return How many commands match. One: the name is filled in. More:
         * the common part is filled in and the matches are printed.
         */
        std::size_t complete() noexcept;

        /**
         * @brief Edit the input with a key: arrows, Home, End, Backspace,
         * Delete, Enter (submit), Tab (complete), Escape (clear the line).
         * @return true when the key was the console's.
         */
        bool key( app::Key key, app::KeyModifiers modifiers ) noexcept;

        /** @brief Typed text, from a keyboard or the soft keyboard. */
        void text( std::string_view utf8 ) noexcept { input_.insert( utf8 ); }

        /**
         * @brief Whether it should be drawn. The Console only keeps the flag:
         * whoever draws it reads it. Hidden at first; `close` and the close
         * button hide it.
         */
        [[nodiscard]] bool shown() const noexcept { return shown_; }
        void show() noexcept { shown_ = true; }
        void hide() noexcept { shown_ = false; }

        [[nodiscard]] const Scrollback &lines() const noexcept { return lines_; }
        [[nodiscard]] Scrollback &lines() noexcept { return lines_; }
        [[nodiscard]] const InputLine &input() const noexcept { return input_; }
        [[nodiscard]] InputLine &input() noexcept { return input_; }
        [[nodiscard]] const Commands &commands() const noexcept { return commands_; }

    private:
        std::array<char, scrollback_bytes> storage_{};
        std::array<char, format_bytes> format_{};
        std::array<char, input_bytes> running_{};
        Scrollback lines_;
        InputLine input_;
        Commands commands_;
        Args args_;
        Mirror mirror_{ nullptr };
        void *mirror_context_{ nullptr };
        bool shown_{ false };
};

} // namespace sq::console
