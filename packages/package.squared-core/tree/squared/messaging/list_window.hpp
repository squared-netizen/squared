#pragma once

#include <cstdint>
#include <optional>

namespace sq::messaging
{

/** @brief Rows of a list: from `first`, `rows` of them. */
struct RowSpan
{
        std::uint32_t first = 0;
        std::uint32_t rows = 0;

        /** @brief The last row, for `do <list>.view first last`; `rows` must not be 0. */
        [[nodiscard]] constexpr std::uint32_t last() const noexcept { return first + rows - 1; }

        friend constexpr bool operator==( RowSpan, RowSpan ) noexcept = default;
};

/**
 * @brief The list widget's end of a virtualized list: which rows to ask the
 * model for. The window is the rows on screen and a screen more on each
 * side; it is asked for again only when the rows on screen come within half
 * a screen of its edge, or the count changes, so a slow scroll asks every
 * half screen and a fling at most once a frame.
 *
 * Holds no rows itself: they are in the latest-value cache, by row, and
 * their fields by uid. The UI thread only.
 */
class ListWindow final
{
    public:
        /** @brief A window of at most `most_rows` rows (`TELEGRAPH_LIST_VIEW`; at least 1). */
        explicit ListWindow( std::uint32_t most_rows ) noexcept;

        /** @brief How many rows the list has, from `state <list>.count`. */
        void set_count( std::uint32_t count ) noexcept;

        /**
         * @brief Each frame: the first row on screen and how many fit. The
         * rows to ask the model for, when they should be asked for; nothing
         * otherwise, or when the list is empty.
         */
        [[nodiscard]] std::optional<RowSpan> visible( std::uint32_t first,
                                                      std::uint32_t rows ) noexcept;

        /** @brief The rows last asked for: the ones to keep bound in the cache. */
        [[nodiscard]] RowSpan window() const noexcept { return window_; }
        [[nodiscard]] std::uint32_t count() const noexcept { return count_; }

    private:
        RowSpan window_;
        std::uint32_t most_ = 1;
        std::uint32_t count_ = 0;
        std::uint32_t screen_ = 0; // rows on screen when last asked
        bool asked_ = false;       // asked since the count last changed
};

static_assert( sizeof( ListWindow ) == 24 );

} // namespace sq::messaging
