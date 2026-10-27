#pragma once

#include <squared/gui/instantiate.hpp>
#include <squared/gui/layout.hpp>
#include <squared/gui/src/size.hpp>
#include <squared/gui/widget.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sq::gui
{

class InputEvent;
class Painter;
class Skin;
class Table;
struct PointerEvent;

/**
 * @brief A list of any length that holds widgets only for the rows it shows:
 * rows of one height, each a table built from one row of a layout, reused
 * as it scrolls. Dragging scrolls it from anywhere in it, a row's buttons
 * included: a finger that moves more than `drag_slop` on a button scrolls
 * instead of tapping.
 *
 * What a row shows is its owner's to set: `set_on_row` is told each time a
 * row's table starts showing another row, or none. The Stage binds a list
 * to a model's list (`"type": "list"` in a layout); the list itself knows
 * only rows by their index, from 0 to `count()`.
 *
 * Allocates when it builds row tables (its first layout, and a taller one:
 * a rotation), and copies its row's nodes and words when it is made; a
 * scroll allocates nothing.
 */
class List final : public Widget
{
    public:
        static constexpr ActorInterfaceId actor_interface_id = 0x4C'49'53'54U;

        /** @brief A row's table that shows no row. */
        static constexpr std::uint32_t no_row = UINT32_MAX;

        /** @brief Row tables built at most; rows that would need more stay empty. */
        static constexpr std::size_t max_rows_built = 64;

        /** @brief How far a finger moves, in logical units, before it scrolls. */
        static constexpr float drag_slop = 8.0F;

        /** @brief What a row asks for until it is measured: 8 rows at most. */
        static constexpr std::uint32_t preferred_rows = 8;

        /**
         * @brief Told that `row`, the list's row table number `slot`, now
         * shows the row `index`, or `no_row`. Called while the list lays out.
         */
        using RowShown = void ( * )( void *context, List &list, std::size_t slot,
                                     std::uint32_t index ) noexcept;

        /**
         * @brief A list whose row is `layout`'s node `row`, with the nodes in
         * it: copied, with the layout's words and numbers, so `layout` need not
         * outlive the list. Rows are built with `hooks` until `set_row_hooks`.
         */
        List( const Layout &layout, std::size_t row, const LayoutHooks &hooks );
        ~List() override;
        List( const List & ) = delete;
        List &operator=( const List & ) = delete;
        List( List && ) = delete;
        List &operator=( List && ) = delete;

        [[nodiscard]] void *actor_interface( ActorInterfaceId id ) noexcept override
        {
            return id == actor_interface_id ? this : Widget::actor_interface( id );
        }

        [[nodiscard]] const void *actor_interface( ActorInterfaceId id ) const noexcept override
        {
            return id == actor_interface_id ? this : Widget::actor_interface( id );
        }

        /**
         * @brief Build rows built from now on with `hooks`, each called with
         * the row's table as its context (`slot_of` says which row table it
         * is); `owner` is kept for them to find (`owner()`). Rows already
         * built keep the hooks they were built with: call it before the first
         * layout.
         */
        void set_row_hooks( const LayoutHooks &hooks, void *owner ) noexcept;

        /** @brief What `set_row_hooks` was given; nullptr before. */
        [[nodiscard]] void *owner() const noexcept { return owner_; }

        /** @brief Be told when a row's table shows another row; nullptr for none. */
        void set_on_row( RowShown shown, void *context ) noexcept;

        /** @brief How many rows there are: the scroll stops at the last. */
        void set_count( std::uint32_t count ) noexcept;
        [[nodiscard]] std::uint32_t count() const noexcept { return count_; }

        /** @brief A row's height, measured from the first row built; 0 before. */
        [[nodiscard]] float row_height() const noexcept { return row_height_; }

        /** @brief The first row on screen, at least partly. */
        [[nodiscard]] std::uint32_t first_row() const noexcept;

        /** @brief How many rows the list's height shows, a part one counted; 0 before its first layout. */
        [[nodiscard]] std::uint32_t rows_on_screen() const noexcept;

        /** @brief How many row tables there are: the rows on screen and one more, at most `max_rows_built`. */
        [[nodiscard]] std::size_t rows_built() const noexcept { return tables_.size(); }

        /** @brief Row table number `slot`. */
        [[nodiscard]] Table &table( std::size_t slot ) noexcept { return *tables_[slot]; }

        /** @brief The row table number `slot` shows: an index, or `no_row`. */
        [[nodiscard]] std::uint32_t row_at( std::size_t slot ) const noexcept
        {
            return rows_[slot];
        }

        /** @brief Which row table `row` is; `rows_built()` when it is none of them. */
        [[nodiscard]] std::size_t slot_of( const Table &row ) const noexcept;

        /** @brief The row table showing row `index`; `rows_built()` when none does. */
        [[nodiscard]] std::size_t slot_showing( std::uint32_t index ) const noexcept;

        /** @brief Scroll to `offset`, in logical units from the top; kept inside the rows. */
        void set_scroll_y( float offset ) noexcept;
        [[nodiscard]] float scroll_y() const noexcept { return scroll_; }

        /** @brief Whether more rows fit than `max_rows_built`: those stay empty. */
        [[nodiscard]] bool short_of_rows() const noexcept { return short_; }

        /** @brief The row, as a layout: its first node is the row's. */
        [[nodiscard]] Layout row() const noexcept;

        /**
         * @brief The row's width, and its height for as many rows as there
         * are, up to `preferred_rows`: a list that should fill its cell grows.
         */
        [[nodiscard]] Size preferred_size( Painter &painter, const Skin &skin ) const override;

        /** @brief Build the row tables it needs, and place the rows on screen. */
        void layout( Painter &painter, const Skin &skin ) override;

        /** @brief Drag to scroll, where no row's widget took the pointer. */
        bool pointer_event( const PointerEvent &event ) override;

    private:
        void measure( Painter &painter, const Skin &skin );
        void build_table();
        void place( Painter &painter, const Skin &skin );
        void scroll_by( float delta ) noexcept;
        [[nodiscard]] float most_scroll() const noexcept;
        void watch( InputEvent &event );
        void tell( std::size_t slot, std::uint32_t index ) noexcept;

        std::vector<LayoutNode> nodes_;
        std::string text_;
        std::vector<float> numbers_;
        LayoutHooks hooks_;
        void *owner_ = nullptr;
        RowShown shown_ = nullptr;
        void *shown_context_ = nullptr;
        std::vector<Table *> tables_;
        std::vector<std::uint32_t> rows_;
        std::optional<std::int64_t> pointer_; // the pointer scrolling or about to
        float row_height_ = 0.0F;
        float row_width_ = 0.0F;
        float scroll_ = 0.0F;
        float start_y_ = 0.0F;
        float last_y_ = 0.0F;
        std::uint32_t count_ = 0;
        bool row_context_ = false; // the hooks take the row's table as context
        bool dragging_ = false;    // the pointer has moved past the slop
        bool short_ = false;
};

} // namespace sq::gui
