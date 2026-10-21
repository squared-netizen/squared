#pragma once

#include <squared/messaging/name.hpp>
#include <squared/messaging/telegram.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sq::messaging
{

/**
 * @brief What a model's list must provide, for an output `Out` (the
 * Telegraph): how many rows, the uid of the item at a row, and a way to send
 * one item's fields. Uids are not 0 (0 means none).
 *
 * ```cpp
 * struct Inventory {
 *     std::vector<Item> items;
 *     std::uint32_t count() const { return std::uint32_t( items.size() ); }
 *     std::uint32_t uid_at( std::uint32_t row ) const { return items[row].uid; }
 *     void publish( sq::Telegraph &t, std::uint32_t uid ) { t.state( names::item_name, uid, ... ); }
 * };
 * ```
 */
template<typename Source, typename Out>
concept ListRows = requires( const Source &rows, Source &source, Out &out, std::uint32_t at ) {
    { rows.count() } -> std::convertible_to<std::uint32_t>;
    { rows.uid_at( at ) } -> std::convertible_to<std::uint32_t>;
    source.publish( out, at );
};

/**
 * @brief A model's list as three plain functions over it: what `ModelList`
 * calls. Made by `list_source`; no virtual call, nothing allocated.
 */
struct ListSource
{
        void *source = nullptr;
        std::uint32_t ( *count )( const void *source ) noexcept = nullptr;
        std::uint32_t ( *uid_at )( const void *source, std::uint32_t row ) noexcept = nullptr;
        /** Send the item's fields, through `out` (the output's context). */
        void ( *publish )( void *source, void *out, std::uint32_t uid ) noexcept = nullptr;
};

/** @brief `source` as a `ListSource`, publishing through an `Out`. */
template<typename Out, typename Source>
    requires ListRows<Source, Out>
[[nodiscard]] ListSource list_source( Source &source ) noexcept
{
    ListSource made;
    made.source = &source;
    made.count = []( const void *s ) noexcept {
        return static_cast<std::uint32_t>( static_cast<const Source *>( s )->count() );
    };
    made.uid_at = []( const void *s, std::uint32_t row ) noexcept {
        return static_cast<std::uint32_t>( static_cast<const Source *>( s )->uid_at( row ) );
    };
    made.publish = []( void *s, void *out, std::uint32_t uid ) noexcept {
        static_cast<Source *>( s )->publish( *static_cast<Out *>( out ), uid );
    };
    return made;
}

/**
 * @brief Where a list's own telegrams go (its count, the uid at a row): the
 * Telegraph, or a test's stand-in. `context` is also what a source's
 * `publish` is given.
 */
struct ListOutput
{
        void *context = nullptr;
        void ( *send )( void *context, const Telegram &telegram ) noexcept = nullptr;
};

/**
 * @brief The model's end of one virtualized list. It answers the list
 * widget's `view` with the rows in it, and after a change sends only what
 * changed on screen: a row whose item changed, and the fields of items new
 * to the view.
 *
 * Telegrams, for a list named `inventory`:
 *
 * | Telegram | Sent |
 * |---|---|
 * | `state inventory.count 300` | at start, and when the count changes |
 * | `state inventory[20] 17` | row 20 shows the item with uid 17 (row 0 is written `inventory`) |
 * | the item's fields (`state item[17].name ...`) | by the source's `publish`, for items new to the view |
 *
 * Over storage its owner gives it: 4 bytes a row the view may hold, and a
 * scratch area as large, which every list on one thread may share. The
 * model's thread only.
 */
class ModelList final
{
    public:
        /**
         * @brief The list `name`, its rows from `source`. `window` is the
         * most rows a view holds (4 bytes each, at least 1); `scratch` is at
         * least as long, and may be shared by every list on this thread.
         */
        ModelList( Name name, ListSource source, std::span<std::uint32_t> window,
                   std::span<std::uint32_t> scratch ) noexcept;

        /** @brief Send the count. Once, before anything else. */
        void start( const ListOutput &out ) noexcept;

        /**
         * @brief The widget shows rows `first` to `last`, inclusive, margin
         * included (`do inventory.view 20 52`). Clamped to the count and to
         * the window's size.
         */
        void view( std::uint32_t first, std::uint32_t last, const ListOutput &out ) noexcept;

        /** @brief The rows changed (a sort, an insert, a removal): the count, then the view. */
        void changed( const ListOutput &out ) noexcept;

        /** @brief One item's fields changed: published again if it is in the view. */
        void changed( std::uint32_t uid, const ListOutput &out ) noexcept;

        [[nodiscard]] Name name() const noexcept { return name_; }
        [[nodiscard]] std::uint32_t count() const noexcept { return count_; }
        /** @brief The rows in the view: from `first()`, `rows()` of them. */
        [[nodiscard]] std::uint32_t first() const noexcept { return first_; }
        [[nodiscard]] std::uint32_t rows() const noexcept { return rows_; }

    private:
        void show( std::uint32_t first, std::uint32_t rows, const ListOutput &out ) noexcept;
        void send_count( const ListOutput &out ) const noexcept;

        ListSource source_;
        Name name_;
        Name count_name_;
        std::span<std::uint32_t> window_;
        std::span<std::uint32_t> scratch_;
        std::uint32_t count_ = 0;
        std::uint32_t first_ = 0;
        std::uint32_t rows_ = 0;
};

static_assert( sizeof( ModelList ) == 96 );

} // namespace sq::messaging
