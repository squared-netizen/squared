#include <squared/gui/layout.hpp>
#include <squared/messaging/src/verb.hpp>
#include <squared/messaging/telegram_text.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace sq::gui
{

namespace
{

[[nodiscard]] bool fits( std::string_view text, std::uint16_t offset ) noexcept
{
    return offset == 0 || offset < text.size();
}

/// A container still open while the nodes are walked.
struct Open
{
        std::size_t end;
        NodeType type;
        std::uint16_t node;
        std::uint16_t children;
};

[[nodiscard]] LayoutCheck check_numbers( const Layout &layout, const LayoutNode &node,
                                         std::uint16_t at ) noexcept
{
    const std::span<const float> numbers = layout.numbers();
    const std::size_t count = node_numbers( node.type );
    if( count == 0 || std::size_t{ node.numbers } + count > numbers.size() ) {
        return { LayoutProblem::bad_numbers, at };
    }
    const std::span<const float> own = numbers.subspan( node.numbers, count );
    // Written so that a NaN fails too.
    if( !( own[0] < own[1] ) ) {
        return { LayoutProblem::bad_numbers, at };
    }
    if( count == 3 && !( own[2] >= 0.0F ) ) {
        return { LayoutProblem::bad_numbers, at }; // a slider's step
    }
    return {};
}

[[nodiscard]] LayoutCheck check_node( const Layout &layout, const LayoutNode &node,
                                      std::uint16_t at ) noexcept
{
    if( node.bind.id != 0 || node.bind_text != 0 ) {
        const std::string_view name = layout.text( node.bind_text );
        const bool named = node.bind.id != 0 && messaging::valid_name( name );
        if( !shows_value( node.type ) || !named || messaging::Name::of( name ) != node.bind ) {
            return { LayoutProblem::bad_bind, at };
        }
    }
    if( node.tap != 0 ) {
        const TapKind kind = read_tap( layout.text( node.tap ) ).kind;
        if( node.type != NodeType::button || kind == TapKind::none || kind == TapKind::invalid ) {
            return { LayoutProblem::bad_tap, at };
        }
    }
    if( node.has_numbers ) {
        if( const LayoutCheck numbers = check_numbers( layout, node, at ); !numbers.ok() ) {
            return numbers;
        }
    }
    if( node.type == NodeType::custom && layout.text( node.text ).empty() ) {
        return { LayoutProblem::no_custom_name, at };
    }
    return {};
}

} // namespace

std::string_view node_type_name( NodeType type ) noexcept
{
    switch( type ) {
        case NodeType::label:
            return "label";
        case NodeType::button:
            return "button";
        case NodeType::check_box:
            return "check_box";
        case NodeType::text_field:
            return "text_field";
        case NodeType::slider:
            return "slider";
        case NodeType::progress_bar:
            return "progress_bar";
        case NodeType::image:
            return "image";
        case NodeType::separator:
            return "separator";
        case NodeType::table:
            return "table";
        case NodeType::scroll:
            return "scroll";
        case NodeType::stack:
            return "stack";
        case NodeType::custom:
            return "custom";
    }
    return {};
}

std::optional<NodeType> node_type_named( std::string_view name ) noexcept
{
    for( std::size_t i = 0; i < node_type_count; ++i ) {
        const auto type = static_cast<NodeType>( i );
        if( !name.empty() && node_type_name( type ) == name ) {
            return type;
        }
    }
    return std::nullopt;
}

std::string_view pane_place_name( PanePlace place ) noexcept
{
    switch( place ) {
        case PanePlace::center:
            return "center";
        case PanePlace::left:
            return "left";
        case PanePlace::right:
            return "right";
        case PanePlace::top:
            return "top";
        case PanePlace::bottom:
            return "bottom";
    }
    return {};
}

std::optional<PanePlace> pane_place_named( std::string_view name ) noexcept
{
    for( std::size_t i = 0; i < pane_place_count; ++i ) {
        const auto place = static_cast<PanePlace>( i );
        if( !name.empty() && pane_place_name( place ) == name ) {
            return place;
        }
    }
    return std::nullopt;
}

Tap read_tap( std::string_view line ) noexcept
{
    if( line.empty() ) {
        return {};
    }
    const std::size_t space = line.find( ' ' );
    const std::string_view word = line.substr( 0, space );
    std::string_view rest;
    if( space != std::string_view::npos ) {
        rest = line.substr( space + 1 );
    }
    const bool sets = word == messaging::verb_text( messaging::Verb::set );
    if( sets || word == messaging::verb_text( messaging::Verb::do_ ) ) {
        return rest.empty() ? Tap{ TapKind::invalid, {} } : Tap{ TapKind::telegram, line };
    }
    for( const TapKind kind : { TapKind::push, TapKind::replace, TapKind::open } ) {
        if( word == tap_word( kind ) ) {
            return messaging::valid_name( rest ) ? Tap{ kind, rest } : Tap{ TapKind::invalid, {} };
        }
    }
    for( const TapKind kind : { TapKind::back, TapKind::close, TapKind::quit } ) {
        if( word == tap_word( kind ) ) {
            return space == std::string_view::npos ? Tap{ kind, {} } : Tap{ TapKind::invalid, {} };
        }
    }
    return { TapKind::invalid, {} };
}

std::string_view tap_word( TapKind kind ) noexcept
{
    switch( kind ) {
        case TapKind::push:
            return "push";
        case TapKind::replace:
            return "replace";
        case TapKind::back:
            return "back";
        case TapKind::open:
            return "open";
        case TapKind::close:
            return "close";
        case TapKind::quit:
            return "quit";
        case TapKind::none:
        case TapKind::telegram:
        case TapKind::invalid:
            break;
    }
    return {};
}

LayoutCheck check( const Layout &layout ) noexcept
{
    const Layout::Nodes nodes = layout.nodes();
    const std::string_view text = layout.text();
    if( nodes.size() > UINT16_MAX ) {
        return { LayoutProblem::bad_tree, UINT16_MAX };
    }
    if( !text.empty() && ( text.front() != '\0' || text.back() != '\0' ) ) {
        return { LayoutProblem::bad_text, 0 };
    }

    std::array<Open, layout_depth> open{};
    std::size_t depth = 0;
    // Close every container that ends at `at`: a scroll holds one node at most.
    const auto close_until = [&open, &depth]( std::size_t at ) -> LayoutCheck {
        while( depth > 0 && open[depth - 1].end <= at ) {
            const Open &closing = open[--depth];
            if( closing.type == NodeType::scroll && closing.children > 1 ) {
                return { LayoutProblem::scroll_holds_one, closing.node };
            }
        }
        return {};
    };

    for( std::size_t i = 0; i < nodes.size(); ++i ) {
        if( const LayoutCheck closed = close_until( i ); !closed.ok() ) {
            return closed;
        }
        const LayoutNode &node = nodes[i];
        const auto at = static_cast<std::uint16_t>( i );
        if( static_cast<std::size_t>( node.type ) >= node_type_count ) {
            return { LayoutProblem::bad_type, at };
        }
        const bool words = fits( text, node.text ) && fits( text, node.style );
        if( !words || !fits( text, node.tap ) || !fits( text, node.bind_text ) ) {
            return { LayoutProblem::bad_text, at };
        }
        const std::size_t limit = depth == 0 ? nodes.size() : open[depth - 1].end;
        const std::size_t end = i + 1 + node.inside;
        if( end > limit ) {
            return { LayoutProblem::bad_tree, at };
        }
        const bool in_table = depth == 0 || open[depth - 1].type == NodeType::table;
        if( depth > 0 ) {
            ++open[depth - 1].children;
        }
        if( node.new_row && !in_table ) {
            return { LayoutProblem::misplaced_row, at };
        }
        if( holds_nodes( node.type ) ) {
            if( depth == open.size() ) {
                return { LayoutProblem::too_deep, at };
            }
            open[depth++] = { end, node.type, at, 0 };
        } else if( node.inside != 0 ) {
            return { LayoutProblem::not_a_container, at };
        }
        if( const LayoutCheck found = check_node( layout, node, at ); !found.ok() ) {
            return found;
        }
    }
    return close_until( SIZE_MAX );
}

std::string_view problem_text( LayoutProblem problem ) noexcept
{
    switch( problem ) {
        case LayoutProblem::none:
            return "none";
        case LayoutProblem::bad_text:
            return "its text is not words each ended by '\\0', or an offset is outside it";
        case LayoutProblem::bad_tree:
            return "a node's inside runs past its container's end";
        case LayoutProblem::too_deep:
            return "containers nested more than 16 deep";
        case LayoutProblem::not_a_container:
            return "nodes inside one that is not a table, scroll or stack";
        case LayoutProblem::scroll_holds_one:
            return "a scroll holds more than one node";
        case LayoutProblem::misplaced_row:
            return "a new row outside a table";
        case LayoutProblem::bad_type:
            return "not a node type";
        case LayoutProblem::bad_bind:
            return "a bind on a node that shows no value, or a name that does not match";
        case LayoutProblem::bad_tap:
            return "a tap on a node that is not a button, or not a tap's line";
        case LayoutProblem::bad_numbers:
            return "numbers missing, on a node that takes none, or a minimum not below its maximum";
        case LayoutProblem::no_custom_name:
            return "a custom node without its type's name";
        case LayoutProblem::unknown_custom:
            return "no custom type of that name";
        case LayoutProblem::unknown_drawable:
            return "the skin has no drawable of that name";
    }
    return "unknown";
}

} // namespace sq::gui
