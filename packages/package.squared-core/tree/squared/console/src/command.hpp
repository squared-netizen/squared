#pragma once

namespace sq::console
{

class Args;
class Console;

/**
 * @brief What a command runs: a plain function, with the context it was added with.
 *
 * A function pointer and a `void*`, not `std::function`: nothing is allocated
 * and the call is direct. A lambda that captures nothing converts to one:
 * `[]( Console &c, const Args &a, void * ) { c.print( a.text( 1 ) ); }`.
 */
using Handler = void ( * )( Console &console, const Args &args, void *context );

/** @brief One entry in the command table. */
struct Command final
{
        /** @brief What is typed; a string literal, or text that outlives the table. */
        const char *name{ nullptr };
        /** @brief One line for `help`, such as "spawn <monster> <count>". */
        const char *help{ nullptr };
        Handler handler{ nullptr };
        void *context{ nullptr };
};

} // namespace sq::console
