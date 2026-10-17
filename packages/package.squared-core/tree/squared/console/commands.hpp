#pragma once

#include <squared/console/src/command.hpp>
#include <squared/console/src/limits.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace sq::console
{

/**
 * @brief The console's fixed command table.
 *
 * `command_slots` entries (`CONSOLE_COMMANDS`, default 64), held inline;
 * adding never allocates. Names and help texts are kept as pointers, so they
 * must outlive the table: string literals are what they are meant to be.
 * Lookup is a linear scan, which for a few dozen names typed by a person is
 * the cheapest structure there is.
 */
class Commands final
{
    public:
        /** @brief Why add() refused. */
        enum class AddResult : unsigned char {
            added,
            table_full,
            duplicate_name,
            invalid_name // empty, or holds a space or a quote
        };

        /** @brief Register a command; see AddResult. */
        [[nodiscard]] AddResult add( const char *name, const char *help, Handler handler,
                                     void *context = nullptr ) noexcept;

        /** @brief The command named `name`, or null. */
        [[nodiscard]] const Command *find( std::string_view name ) const noexcept;

        /** @brief Every registered command, in the order added. */
        [[nodiscard]] std::span<const Command> all() const noexcept
        {
            return { table_.data(), count_ };
        }

    private:
        std::array<Command, command_slots> table_{};
        std::size_t count_{ 0 };
};

} // namespace sq::console
