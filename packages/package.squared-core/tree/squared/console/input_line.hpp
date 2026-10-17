#pragma once

#include <squared/console/scrollback.hpp>
#include <squared/console/src/limits.hpp>

#include <array>
#include <cstddef>
#include <string_view>

namespace sq::console
{

/**
 * @brief The line being typed: text, a cursor, and the history of earlier lines.
 *
 * Text is UTF-8; the cursor moves and deletes whole characters. Up to
 * `input_bytes` (256) are held, and `history_bytes` (2 KB) of earlier lines,
 * all inline: editing never allocates.
 */
class InputLine final
{
    public:
        InputLine() noexcept;
        ~InputLine() = default;
        // The history views the line's own storage, so the line cannot move.
        InputLine( const InputLine & ) = delete;
        InputLine &operator=( const InputLine & ) = delete;
        InputLine( InputLine && ) = delete;
        InputLine &operator=( InputLine && ) = delete;

        /** @brief Insert at the cursor. Control characters are dropped; what does not fit is cut at a character boundary. */
        void insert( std::string_view utf8 ) noexcept;
        /** @brief Delete the character before the cursor (Backspace). */
        void erase_before() noexcept;
        /** @brief Delete the character after the cursor (Delete). */
        void erase_after() noexcept;
        void left() noexcept;
        void right() noexcept;
        void home() noexcept { cursor_ = 0; }
        void end() noexcept { cursor_ = length_; }
        /** @brief Replace the text, cursor at its end. */
        void set( std::string_view utf8 ) noexcept;
        /** @brief Empty the line; the history is kept. */
        void clear() noexcept;

        /** @brief Show the previous history line (Up); the text being typed is kept for next(). */
        void previous() noexcept;
        /** @brief Show the next history line (Down), and at the end the text that was being typed. */
        void next() noexcept;
        /** @brief Add the current text to the history, unless empty or the same as the newest. */
        void remember() noexcept;

        [[nodiscard]] std::string_view text() const noexcept { return { buffer_.data(), length_ }; }
        /** @brief Cursor position in bytes, always at a character boundary. */
        [[nodiscard]] std::size_t cursor() const noexcept { return cursor_; }
        [[nodiscard]] const Scrollback &history() const noexcept { return history_; }

    private:
        std::array<char, input_bytes> buffer_{};
        std::array<char, input_bytes> draft_{};
        std::array<char, history_bytes> history_storage_{};
        Scrollback history_;
        std::size_t length_{ 0 };
        std::size_t cursor_{ 0 };
        std::size_t draft_length_{ 0 };
        std::size_t browsing_{ 0 }; // 0: editing; n: showing the n-th newest history line
};

} // namespace sq::console
