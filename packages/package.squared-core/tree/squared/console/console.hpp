#pragma once

#include <squared/app/src/key.hpp>
#include <squared/app/src/key_modifiers.hpp>
#include <squared/console/args.hpp>
#include <squared/console/commands.hpp>
#include <squared/console/groups.hpp>
#include <squared/console/input_line.hpp>
#include <squared/console/scrollback.hpp>

#include <array>
#include <cstddef>
#include <initializer_list>
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
        /** @brief An empty, hidden console with its built-ins: `help`, `clear`, `close`, `dump`. */
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
         * @brief Add `line` (a whole command line, `logcat 100`) to the group
         * `group` for `dump`, making the group if it is new. See Groups::add.
         */
        Groups::AddResult add_to_group( const char *group, const char *line ) noexcept
        {
            return groups_.add( group, line );
        }

        /** @brief Add each of `lines` to `group`; the first refusal, or `added`. */
        Groups::AddResult add_group( const char *group,
                                     std::initializer_list<const char *> lines ) noexcept;

        /**
         * @brief Run the next line `dump` queued, if any: once a frame, so a
         * long group never holds a frame up, and the bridge sends between.
         */
        void update() noexcept;

        /** @brief Whether `dump` has lines still to run: the loop should not sleep long. */
        [[nodiscard]] bool dumping() const noexcept { return queued_ != 0; }

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
        [[nodiscard]] const Groups &groups() const noexcept { return groups_; }

        /** @brief What queue_group() did. */
        struct Queued final
        {
                /** @brief False when there is no group of that name. */
                bool found{ false };
                /** @brief Lines queued now. */
                std::size_t queued{ 0 };
                /** @brief Lines left out: the queue was full (`CONSOLE_GROUP_LINES`). */
                std::size_t full{ 0 };
        };

        /**
         * @brief Queue each line of `group` that is not queued already, for
         * update() to run; `all` queues every group, in the order made.
         */
        Queued queue_group( std::string_view group ) noexcept;

        /** @brief Forget the queued lines; how many there were. */
        std::size_t stop_dump() noexcept;

    private:
        std::array<char, scrollback_bytes> storage_{};
        std::array<char, format_bytes> format_{};
        std::array<char, input_bytes> running_{};
        Scrollback lines_;
        InputLine input_;
        Commands commands_;
        Groups groups_;
        // dump's queue: a ring of line pointers, run one a frame.
        std::array<const char *, group_line_slots> queue_{};
        std::size_t queue_head_{ 0 };
        std::size_t queued_{ 0 };
        std::size_t dump_run_{ 0 };
        Args args_;
        Mirror mirror_{ nullptr };
        void *mirror_context_{ nullptr };
        bool shown_{ false };
};

} // namespace sq::console
