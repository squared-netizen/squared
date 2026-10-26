#include <squared/gui/layout_writer.hpp>
#include <squared/messaging/name_table.hpp>
#include <squared/messaging/src/data_ref.hpp>
#include <squared/messaging/src/value_type.hpp>
#include <squared/messaging/telegram_text.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

namespace sq::gui
{

namespace
{

[[nodiscard]] constexpr TapKind kind_of( Navigate how ) noexcept
{
    switch( how ) {
        case Navigate::push:
            return TapKind::push;
        case Navigate::replace:
            return TapKind::replace;
        case Navigate::back:
            return TapKind::back;
        case Navigate::open:
            return TapKind::open;
        case Navigate::close:
            return TapKind::close;
        case Navigate::quit:
            return TapKind::quit;
    }
    return TapKind::invalid;
}

/// A line longer than this is not a tap: a value that long belongs in the model.
constexpr std::size_t tap_line_bytes = 512;

} // namespace

// ---- LayoutWriter ----------------------------------------------------------

LayoutWriter::LayoutWriter( std::span<LayoutNode> nodes, std::span<char> text,
                            std::span<float> numbers, std::string_view name,
                            LayoutKind kind ) noexcept
    // Indices and offsets are 16 bits; the root's index is the largest but one.
    : nodes_( nodes.first( std::min<std::size_t>( nodes.size(), root ) ) )
    , text_( text.first( std::min<std::size_t>( text.size(), UINT16_MAX ) ) )
    , numbers_( numbers.first( std::min<std::size_t>( numbers.size(), UINT16_MAX ) ) )
    , name_( name )
    , kind_( kind )
{
    clear();
}

void LayoutWriter::clear() noexcept
{
    count_ = 0;
    depth_ = 0;
    root_children_ = 0;
    root_row_next_ = false;
    numbers_used_ = 0;
    complete_ = true;
    text_used_ = 0;
    if( !text_.empty() ) {
        text_[0] = '\0'; // offset 0 is "none"
        text_used_ = 1;
    }
}

Layout LayoutWriter::layout() const noexcept
{
    const std::string_view text( text_.data(), text_used_ );
    const std::span<const float> numbers = numbers_.first( numbers_used_ );
    return Layout( name_, kind_, nodes_.first( count_ ), text, numbers, pane_ );
}

LayoutWriter &LayoutWriter::row() noexcept
{
    root_row_next_ = true;
    return *this;
}

NodeWriter LayoutWriter::add_custom( std::string_view name ) noexcept
{
    return add_to( root, NodeType::custom, name );
}

std::uint16_t LayoutWriter::put_text( std::string_view text ) noexcept
{
    text = text.substr( 0, text.find( '\0' ) ); // a '\0' would end the words early anyway
    if( text.empty() ) {
        return 0;
    }
    if( text_used_ + text.size() + 1 > text_.size() ) {
        refuse();
        return 0;
    }
    const auto offset = static_cast<std::uint16_t>( text_used_ );
    std::memcpy( text_.data() + text_used_, text.data(), text.size() );
    text_used_ += text.size();
    text_[text_used_++] = '\0';
    return offset;
}

LayoutWriter::Open *LayoutWriter::open_entry( std::uint16_t node ) noexcept
{
    for( std::size_t at = depth_; at > 0; --at ) {
        if( open_[at - 1].node == node ) {
            return &open_[at - 1];
        }
    }
    return nullptr;
}

NodeWriter LayoutWriter::add_to( std::uint16_t parent, NodeType type,
                                 std::string_view text ) noexcept
{
    const NodeWriter unwritten( this, NodeWriter::none );
    std::uint16_t *children = &root_children_;
    bool *row_next = &root_row_next_;
    if( parent == NodeWriter::none ) {
        refuse(); // into a node that was not written
        return unwritten;
    }
    if( parent == root ) {
        depth_ = 0; // every container still open is finished
    } else {
        // The parent must still take nodes; those opened after it are finished.
        std::size_t at = depth_;
        while( at > 0 && open_[at - 1].node != parent ) {
            --at;
        }
        if( at == 0 ) {
            refuse(); // finished, or not a container
            return unwritten;
        }
        depth_ = static_cast<std::uint16_t>( at );
        Open &entry = open_[at - 1];
        if( nodes_[parent].type == NodeType::scroll && entry.children != 0 ) {
            refuse(); // a scroll holds one node
            return unwritten;
        }
        children = &entry.children;
        row_next = &entry.row_next;
    }
    if( count_ == nodes_.size() || ( holds_nodes( type ) && depth_ == open_.size() ) ) {
        refuse();
        return unwritten;
    }
    LayoutNode node{};
    node.type = type;
    node.text = put_text( text );
    if( *row_next ) {
        node.new_row = *children != 0; // a first row needs no start
        *row_next = false;
    }
    const std::uint16_t index = count_++;
    nodes_[index] = node;
    ++*children;
    for( std::size_t at = 0; at < depth_; ++at ) {
        ++nodes_[open_[at].node].inside;
    }
    if( holds_nodes( type ) ) {
        open_[depth_++] = { index, 0, false };
    }
    return { this, index };
}

// ---- NodeWriter ------------------------------------------------------------

LayoutNode *NodeWriter::node() noexcept
{
    return index_ == none ? nullptr : &writer_->nodes_[index_];
}

bool NodeWriter::written() const noexcept
{
    return index_ != none;
}

NodeWriter NodeWriter::add_custom( std::string_view name ) noexcept
{
    return writer_->add_to( index_, NodeType::custom, name );
}

NodeWriter &NodeWriter::row() noexcept
{
    LayoutWriter::Open *open = index_ == none ? nullptr : writer_->open_entry( index_ );
    if( open == nullptr || writer_->nodes_[index_].type != NodeType::table ) {
        writer_->refuse(); // only a table still taking nodes has rows
        return *this;
    }
    open->row_next = true;
    return *this;
}

NodeWriter &NodeWriter::grow() noexcept
{
    return grow_x().grow_y();
}

NodeWriter &NodeWriter::grow_x() noexcept
{
    if( LayoutNode *n = node() ) {
        n->grow_x = true;
    }
    return *this;
}

NodeWriter &NodeWriter::grow_y() noexcept
{
    if( LayoutNode *n = node() ) {
        n->grow_y = true;
    }
    return *this;
}

NodeWriter &NodeWriter::fill() noexcept
{
    return fill_x().fill_y();
}

NodeWriter &NodeWriter::fill_x() noexcept
{
    if( LayoutNode *n = node() ) {
        n->fill_x = true;
    }
    return *this;
}

NodeWriter &NodeWriter::fill_y() noexcept
{
    if( LayoutNode *n = node() ) {
        n->fill_y = true;
    }
    return *this;
}

NodeWriter &NodeWriter::pad( float space ) noexcept
{
    if( LayoutNode *n = node() ) {
        n->pad = space;
    }
    return *this;
}

NodeWriter &NodeWriter::align( Alignment x, Alignment y ) noexcept
{
    if( LayoutNode *n = node() ) {
        n->align_x = x;
        n->align_y = y;
    }
    return *this;
}

NodeWriter &NodeWriter::span( std::uint8_t columns ) noexcept
{
    if( LayoutNode *n = node() ) {
        n->span = std::max<std::uint8_t>( columns, 1 );
    }
    return *this;
}

NodeWriter &NodeWriter::style( std::string_view name ) noexcept
{
    if( LayoutNode *n = node() ) {
        n->style = writer_->put_text( name );
    }
    return *this;
}

NodeWriter &NodeWriter::vertical() noexcept
{
    if( LayoutNode *n = node() ) {
        if( n->type == NodeType::separator ) {
            n->vertical = true;
        } else {
            writer_->refuse();
        }
    }
    return *this;
}

float *NodeWriter::numbers() noexcept
{
    LayoutNode *n = node();
    if( n == nullptr ) {
        return nullptr;
    }
    const std::size_t count = node_numbers( n->type );
    if( count == 0 ) {
        writer_->refuse();
        return nullptr;
    }
    if( !n->has_numbers ) {
        if( writer_->numbers_used_ + count > writer_->numbers_.size() ) {
            writer_->refuse();
            return nullptr;
        }
        n->numbers = static_cast<std::uint16_t>( writer_->numbers_used_ );
        n->has_numbers = true;
        float *first = writer_->numbers_.data() + writer_->numbers_used_;
        first[0] = 0.0F; // the widgets' own defaults
        first[1] = 1.0F;
        if( count == 3 ) {
            first[2] = 0.0F;
        }
        writer_->numbers_used_ += count;
    }
    return writer_->numbers_.data() + n->numbers;
}

NodeWriter &NodeWriter::range( float minimum, float maximum ) noexcept
{
    if( !( minimum < maximum ) ) {
        writer_->refuse();
        return *this;
    }
    if( float *at = numbers() ) {
        at[0] = minimum;
        at[1] = maximum;
    }
    return *this;
}

NodeWriter &NodeWriter::step( float step ) noexcept
{
    const LayoutNode *n = node();
    if( n != nullptr && ( n->type != NodeType::slider || !( step >= 0.0F ) ) ) {
        writer_->refuse();
        return *this;
    }
    if( float *at = numbers() ) {
        at[2] = step;
    }
    return *this;
}

NodeWriter &NodeWriter::bind_name( messaging::Name name, std::string_view text ) noexcept
{
    LayoutNode *n = node();
    if( n == nullptr ) {
        return *this;
    }
    if( !shows_value( n->type ) ) {
        writer_->refuse();
        return *this;
    }
    n->bind_text = writer_->put_text( text );
    n->bind = n->bind_text != 0 ? name : messaging::Name{};
    return *this;
}

NodeWriter &NodeWriter::tap( std::string_view line ) noexcept
{
    LayoutNode *n = node();
    if( n == nullptr ) {
        return *this;
    }
    const TapKind kind = read_tap( line ).kind;
    if( n->type != NodeType::button || kind == TapKind::none || kind == TapKind::invalid ) {
        writer_->refuse();
        return *this;
    }
    n->tap = writer_->put_text( line );
    return *this;
}

NodeWriter &NodeWriter::tap( Navigate how, std::string_view target ) noexcept
{
    const std::string_view word = tap_word( kind_of( how ) );
    std::array<char, tap_line_bytes> line{};
    if( word.size() + 1 + target.size() > line.size() ) {
        writer_->refuse();
        return *this;
    }
    std::memcpy( line.data(), word.data(), word.size() );
    std::size_t size = word.size();
    if( !target.empty() ) {
        line[size++] = ' ';
        std::memcpy( line.data() + size, target.data(), target.size() );
        size += target.size();
    }
    return tap( std::string_view( line.data(), size ) );
}

NodeWriter &NodeWriter::tap_telegram( const messaging::Telegram &telegram,
                                      std::span<const std::byte> data,
                                      std::string_view name ) noexcept
{
    // The telegram's own text form, from the one writer: a name table of
    // one, so the line names the key rather than its hash.
    std::array<std::byte, 16 + 256> storage{};
    messaging::NameTable names( storage );
    static_cast<void>( names.add( name ) );
    // A text or uid list is the whole of `data`: say so, as the Telegraph
    // does when it copies one into its ring.
    const messaging::ValueType type = telegram.type();
    messaging::Telegram written = telegram;
    if( type == messaging::ValueType::text || type == messaging::ValueType::uids ) {
        const messaging::DataRef all{ 0, static_cast<std::uint32_t>( data.size() ) };
        written = messaging::Telegram::data( telegram.verb(), telegram.address(), type, all );
    }
    std::array<char, tap_line_bytes> line{};
    const std::size_t size = messaging::write_telegram( written, data, names, line );
    if( size == 0 ) {
        writer_->refuse();
        return *this;
    }
    return tap( std::string_view( line.data(), size ) );
}

} // namespace sq::gui
