#pragma once

#include <squared/console/src/limits.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace sq::console
{

/** @brief One command line of a group: which group, and the line. */
struct GroupLine final
{
        std::uint16_t group{ 0 };
        /** @brief A command line, as typed; a string literal, or text that outlives the table. */
        const char *line{ nullptr };
};

/**
 * @brief Named groups of command lines, for `dump`: `sys` runs every system
 * command, `memory` the memory ones, and so on.
 *
 * A fixed table, like the command table: `group_slots` names
 * (`CONSOLE_GROUPS`, 16) and `group_line_slots` lines among them
 * (`CONSOLE_GROUP_LINES`, 128), held inline; adding never allocates. A line
 * is a whole command line (`logcat 100`), so a group can pass arguments.
 * Names and lines are kept as pointers: string literals are what they are
 * meant to be.
 */
class Groups final
{
    public:
        /** @brief Why add() refused. */
        enum class AddResult : unsigned char {
            added,
            /** Every group name is taken. */
            groups_full,
            /** Every line is taken. */
            lines_full,
            /**
             * The name is empty, holds a space or a quote, or is `all` or
             * `stop` (dump's own words); or the line is empty, or a `dump`.
             */
            invalid_name,
        };

        /**
         * @brief Add `line` to the group `group`, making the group if it is new.
         * A line already in the group is not added twice (and is `added`).
         */
        [[nodiscard]] AddResult add( const char *group, const char *line ) noexcept;

        /** @brief The number of the group named `name`, or nothing. */
        [[nodiscard]] std::optional<std::size_t> find( std::string_view name ) const noexcept;

        /** @brief Every group's name, in the order made. */
        [[nodiscard]] std::span<const char *const> names() const noexcept
        {
            return { names_.data(), name_count_ };
        }

        /** @brief Every line, in the order added; `group` says whose. */
        [[nodiscard]] std::span<const GroupLine> lines() const noexcept
        {
            return { lines_.data(), line_count_ };
        }

    private:
        std::array<const char *, group_slots> names_{};
        std::array<GroupLine, group_line_slots> lines_{};
        std::size_t name_count_{ 0 };
        std::size_t line_count_{ 0 };
};

} // namespace sq::console
