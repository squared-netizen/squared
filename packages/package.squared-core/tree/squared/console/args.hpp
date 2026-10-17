#pragma once

#include <squared/console/src/limits.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sq::console
{

/**
 * @brief The words of a command line, as a command receives them.
 *
 * Word 0 is the command's name. Words are separated by spaces; a word in
 * double quotes may hold spaces (`say "two words"`). Every word is a view into
 * the input line, valid while the command runs: copy what you keep.
 *
 * ```cpp
 * // spawn goblin 3
 * const std::string_view monster = args.text( 1 );
 * const std::int64_t count = args.integer( 2, 1 );
 * ```
 */
class Args final
{
    public:
        /** @brief Split `line` into words; false if it has more than max_words. */
        [[nodiscard]] bool parse( std::string_view line ) noexcept;

        /** @brief Words, the name included; 0 for an empty line. */
        [[nodiscard]] std::size_t size() const noexcept { return count_; }

        /** @brief Word `index`, or empty if there is none. */
        [[nodiscard]] std::string_view text( std::size_t index ) const noexcept;

        /**
         * @brief Word `index` as a whole number (decimal, or hex with 0x).
         * @return `fallback` if the word is missing or not a number.
         */
        [[nodiscard]] std::int64_t integer( std::size_t index,
                                            std::int64_t fallback = 0 ) const noexcept;

        /**
         * @brief Word `index` as a real number (`1.5`, `-2e3`).
         * @return `fallback` if the word is missing or not a number.
         */
        [[nodiscard]] double real( std::size_t index, double fallback = 0.0 ) const noexcept;

    private:
        std::array<std::string_view, max_words> words_{};
        std::size_t count_{ 0 };
};

} // namespace sq::console
