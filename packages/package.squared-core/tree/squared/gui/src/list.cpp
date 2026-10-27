#include <squared/gui/list.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/src/detail/build_row.hpp>
#include <squared/gui/src/input_event.hpp>
#include <squared/gui/src/input_type.hpp>
#include <squared/gui/src/painter.hpp>
#include <squared/gui/src/pointer_action.hpp>
#include <squared/gui/src/pointer_event.hpp>
#include <squared/gui/src/size.hpp>
#include <squared/gui/table.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace sq::gui
{

List::List( const Layout &layout, std::size_t row, const LayoutHooks &hooks )
    : text_( layout.text() )
    , numbers_( layout.numbers().begin(), layout.numbers().end() )
    , hooks_( hooks )
{
    const Layout::Nodes nodes = layout.nodes();
    if( row < nodes.size() ) {
        const std::size_t end = std::min( row + 1 + nodes[row].inside, nodes.size() );
        const Layout::Nodes inside = nodes.subspan( row, end - row );
        nodes_.assign( inside.begin(), inside.end() );
        nodes_.front().new_row = false; // the first and only node of its table
    }
    // Watch what the rows' widgets are given, to scroll when a finger drags.
    const auto watching = [this]( InputEvent &event ) { watch( event ); };
    static_cast<void>( add_input_listener( watching, true ) );
}

List::~List() = default;

void List::set_row_hooks( const LayoutHooks &hooks, void *owner ) noexcept
{
    hooks_ = hooks;
    owner_ = owner;
    row_context_ = true;
}

void List::set_on_row( RowShown shown, void *context ) noexcept
{
    shown_ = shown;
    shown_context_ = context;
}

void List::set_count( std::uint32_t count ) noexcept
{
    if( count != count_ ) {
        count_ = count;
        invalidate_layout();
    }
}

Layout List::row() const noexcept
{
    return { {}, LayoutKind::screen, nodes_, text_, numbers_ };
}

std::uint32_t List::first_row() const noexcept
{
    if( row_height_ <= 0.0F ) {
        return 0;
    }
    return static_cast<std::uint32_t>( scroll_ / row_height_ );
}

std::uint32_t List::rows_on_screen() const noexcept
{
    if( row_height_ <= 0.0F || height() <= 0.0F ) {
        return 0;
    }
    // The first may show only its lower part, so one more can show at the foot.
    const float below = scroll_ - static_cast<float>( first_row() ) * row_height_;
    return static_cast<std::uint32_t>( std::ceil( ( height() + below ) / row_height_ ) );
}

std::size_t List::slot_of( const Table &row ) const noexcept
{
    const auto found = std::find( tables_.begin(), tables_.end(), &row );
    return static_cast<std::size_t>( found - tables_.begin() );
}

std::size_t List::slot_showing( std::uint32_t index ) const noexcept
{
    if( index == no_row ) {
        return tables_.size();
    }
    const auto found = std::find( rows_.begin(), rows_.end(), index );
    return static_cast<std::size_t>( found - rows_.begin() );
}

float List::most_scroll() const noexcept
{
    const float rows = static_cast<float>( count_ ) * row_height_;
    return std::max( 0.0F, rows - height() );
}

void List::set_scroll_y( float offset ) noexcept
{
    wanted_ = -1.0F; // scrolled since: what was to be put back is forgotten
    const float kept = std::clamp( offset, 0.0F, most_scroll() );
    if( kept != scroll_ ) {
        scroll_ = kept;
        invalidate_layout();
    }
}

void List::restore_scroll( float offset ) noexcept
{
    wanted_ = std::max( 0.0F, offset );
    invalidate_layout();
}

void List::scroll_by( float delta ) noexcept
{
    set_scroll_y( scroll_ + delta );
}

void List::build_table()
{
    auto made = std::make_unique<Table>();
    Table &table = *made;
    static_cast<void>( add_actor( std::move( made ) ) );
    tables_.push_back( &table );
    rows_.push_back( no_row );
    table.set_visible( false );
}

void List::measure( Painter &painter, const Skin &skin )
{
    if( !tables_.empty() || nodes_.empty() ) {
        return;
    }
    build_table();
    Table &first = *tables_.front();
    LayoutHooks hooks = hooks_;
    if( row_context_ ) {
        hooks.context = &first;
    }
    static_cast<void>( detail::build_row( row(), first, skin, hooks ) );
    const Size size = first.preferred_size( painter, skin );
    row_width_ = size.width;
    row_height_ = std::max( size.height, 1.0F );
}

Size List::preferred_size( Painter &painter, const Skin &skin ) const
{
    // Measuring builds the first row: a List is never made const.
    auto &self = const_cast<List &>( *this ); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    self.measure( painter, skin );
    const std::uint32_t rows = std::clamp<std::uint32_t>( count_, 1, preferred_rows );
    return { row_width_, row_height_ * static_cast<float>( rows ) };
}

void List::layout( Painter &painter, const Skin &skin )
{
    measure( painter, skin );
    if( tables_.empty() ) {
        return;
    }
    // However far it is scrolled, no more rows than this show at once: one
    // more table is never needed while it scrolls.
    const auto fit = static_cast<std::size_t>( std::ceil( height() / row_height_ ) );
    const std::size_t wanted = fit + 1;
    short_ = wanted > max_rows_built;
    while( tables_.size() < std::min( wanted, max_rows_built ) ) {
        build_table();
        Table &table = *tables_.back();
        LayoutHooks hooks = hooks_;
        if( row_context_ ) {
            hooks.context = &table;
        }
        static_cast<void>( detail::build_row( row(), table, skin, hooks ) );
    }
    if( wanted_ >= 0.0F ) {
        // As far as the rows go: all the way, once the count reaches it.
        scroll_ = wanted_;
        if( most_scroll() >= wanted_ ) {
            wanted_ = -1.0F;
        }
    }
    scroll_ = std::clamp( scroll_, 0.0F, most_scroll() );
    place( painter, skin );
}

void List::tell( std::size_t slot, std::uint32_t index ) noexcept
{
    rows_[slot] = index;
    if( shown_ != nullptr ) {
        shown_( shown_context_, *this, slot, index );
    }
}

void List::place( Painter &painter, const Skin &skin )
{
    // Row r is in table r % n: scrolling moves a row to another table only
    // when it comes on screen.
    const std::size_t n = tables_.size();
    const std::uint32_t first = first_row();
    for( std::size_t slot = 0; slot < n; ++slot ) {
        const std::size_t offset = ( slot + n - first % n ) % n;
        const std::uint64_t index = std::uint64_t{ first } + offset;
        Table &table = *tables_[slot];
        const float top = static_cast<float>( index ) * row_height_ - scroll_;
        if( index >= count_ || top >= height() ) {
            if( rows_[slot] != no_row ) {
                tell( slot, no_row );
            }
            table.set_visible( false );
            continue;
        }
        const auto row = static_cast<std::uint32_t>( index );
        if( rows_[slot] != row ) {
            tell( slot, row );
        }
        table.set_visible( true );
        table.set_bounds( 0.0F, top, width(), row_height_ );
        table.validate_layout( painter, skin );
    }
}

bool List::pointer_event( const PointerEvent &event )
{
    if( !enabled() ) {
        return false;
    }
    if( event.action == PointerAction::down ) {
        pointer_ = event.pointer_id;
        last_y_ = event.y;
        return true;
    }
    if( !pointer_ || *pointer_ != event.pointer_id ) {
        return false;
    }
    if( event.action == PointerAction::move ) {
        scroll_by( last_y_ - event.y );
        last_y_ = event.y;
        return true;
    }
    pointer_.reset();
    return true;
}

void List::watch( InputEvent &event )
{
    if( event.target() == this ) {
        return; // its own: pointer_event
    }
    switch( event.type ) {
        case InputType::pointer_down:
            pointer_ = event.pointer_id;
            start_y_ = event.stage_y;
            last_y_ = event.stage_y;
            dragging_ = false;
            return;
        case InputType::pointer_move:
            if( !pointer_ || *pointer_ != event.pointer_id ) {
                return;
            }
            if( !dragging_ ) {
                if( std::abs( event.stage_y - start_y_ ) < drag_slop ) {
                    return; // still a tap, maybe
                }
                // A drag: the widget under the finger lets go, the list scrolls.
                dragging_ = true;
                event.type = InputType::pointer_cancel;
                scroll_by( last_y_ - event.stage_y );
                last_y_ = event.stage_y;
                return;
            }
            scroll_by( last_y_ - event.stage_y );
            last_y_ = event.stage_y;
            event.handle();
            event.stop();
            return;
        case InputType::pointer_up:
        case InputType::pointer_cancel:
            if( pointer_ && *pointer_ == event.pointer_id && dragging_ ) {
                event.handle();
                event.stop(); // the widget let go already
            }
            pointer_.reset();
            dragging_ = false;
            return;
        default:
            return;
    }
}

} // namespace sq::gui
