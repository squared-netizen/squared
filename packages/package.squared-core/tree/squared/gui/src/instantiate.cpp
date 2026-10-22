#include <squared/gui/button.hpp>
#include <squared/gui/cell.hpp>
#include <squared/gui/check_box.hpp>
#include <squared/gui/image.hpp>
#include <squared/gui/instantiate.hpp>
#include <squared/gui/label.hpp>
#include <squared/gui/progress_bar.hpp>
#include <squared/gui/scroll_pane.hpp>
#include <squared/gui/separator.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/slider.hpp>
#include <squared/gui/src/direction.hpp>
#include <squared/gui/stack.hpp>
#include <squared/gui/table.hpp>
#include <squared/gui/text_field.hpp>
#include <squared/gui/widget.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace sq::gui
{

namespace
{

/// A button's tap: the hook, and its line, copied (the layout may be gone).
struct TapCall
{
        void ( *tapped )( void *context, std::string_view line ) noexcept;
        void *context;
        std::string line;

        void operator()() const { tapped( context, line ); }
};

/// One instantiation: the layout, where widgets come from, and the first
/// thing that built short.
class Builder final
{
    public:
        Builder( const Layout &layout, const Skin &skin, const LayoutHooks &hooks ) noexcept
            : layout_( layout )
            , nodes_( layout.nodes() )
            , skin_( skin )
            , hooks_( hooks )
        {}

        /// The node after the one at `at` and everything inside it.
        [[nodiscard]] std::size_t after( std::size_t at ) const noexcept
        {
            return at + 1 + nodes_[at].inside;
        }

        /// Each node directly inside [first, end) into `table`, in its cell.
        void fill_table( Table &table, std::size_t first, std::size_t end )
        {
            for( std::size_t at = first; at < end; at = after( at ) ) {
                const LayoutNode &node = nodes_[at];
                if( node.new_row ) {
                    table.row();
                }
                place( table.add( make( at ) ), node );
            }
        }

        [[nodiscard]] LayoutCheck result() const noexcept { return short_; }

    private:
        static void place( Cell &cell, const LayoutNode &node ) noexcept
        {
            if( node.grow_x ) {
                cell.grow_x();
            }
            if( node.grow_y ) {
                cell.grow_y();
            }
            if( node.fill_x ) {
                cell.fill_x();
            }
            if( node.fill_y ) {
                cell.fill_y();
            }
            if( node.pad >= 0.0F ) {
                cell.pad( node.pad );
            }
            if( node.span > 1 ) {
                cell.column_span( node.span );
            }
            cell.align( node.align_x, node.align_y );
        }

        void built_short( LayoutProblem problem, std::size_t at ) noexcept
        {
            if( short_.ok() ) {
                short_ = { problem, static_cast<std::uint16_t>( at ) };
            }
        }

        /// A node's number `which`, or `otherwise` when it has none of its own.
        [[nodiscard]] float number( const LayoutNode &node, std::size_t which,
                                    float otherwise ) const
        {
            if( !node.has_numbers ) {
                return otherwise;
            }
            return layout_.numbers()[std::size_t{ node.numbers } + which];
        }

        [[nodiscard]] std::unique_ptr<Widget> make( std::size_t at )
        {
            const LayoutNode &node = nodes_[at];
            std::unique_ptr<Widget> made = make_widget( node, at );
            if( node.bind.id != 0 && hooks_.bound != nullptr ) {
                hooks_.bound( hooks_.context, layout_, node, *made );
            }
            return made;
        }

        [[nodiscard]] std::unique_ptr<Widget> make_widget( const LayoutNode &node, std::size_t at )
        {
            const std::string text( layout_.text( node.text ) );
            const std::string style( layout_.text( node.style ) );
            const std::size_t end = after( at );
            switch( node.type ) {
                case NodeType::label: {
                    auto label = std::make_unique<Label>( text );
                    if( !style.empty() ) {
                        label->set_style( style );
                    }
                    return label;
                }
                case NodeType::button: {
                    auto button = std::make_unique<Button>( text );
                    if( !style.empty() ) {
                        button->set_style( style );
                    }
                    if( node.tap != 0 ) {
                        button->set_on_click( tap_of( layout_.text( node.tap ) ) );
                    }
                    return button;
                }
                case NodeType::check_box: {
                    auto box = std::make_unique<CheckBox>( text );
                    if( !style.empty() ) {
                        box->set_check_style( style );
                    }
                    return box;
                }
                case NodeType::text_field: {
                    auto field = std::make_unique<TextField>( text );
                    if( !style.empty() ) {
                        field->set_style( style );
                    }
                    return field;
                }
                case NodeType::slider: {
                    const float minimum = number( node, 0, 0.0F );
                    const float maximum = number( node, 1, 1.0F );
                    auto slider = std::make_unique<Slider>( minimum, maximum, minimum );
                    if( const float step = number( node, 2, 0.0F ); step > 0.0F ) {
                        slider->set_step( step );
                    }
                    if( !style.empty() ) {
                        slider->set_style( style );
                    }
                    return slider;
                }
                case NodeType::progress_bar: {
                    const float minimum = number( node, 0, 0.0F );
                    const float maximum = number( node, 1, 1.0F );
                    auto bar = std::make_unique<ProgressBar>( minimum, maximum, minimum );
                    if( !style.empty() ) {
                        bar->set_style( style );
                    }
                    return bar;
                }
                case NodeType::image: {
                    DrawablePtr drawable = skin_.drawable( text );
                    if( !drawable ) {
                        built_short( LayoutProblem::unknown_drawable, at );
                    }
                    return std::make_unique<Image>( std::move( drawable ) );
                }
                case NodeType::separator: {
                    Direction runs = Direction::horizontal;
                    if( node.vertical ) {
                        runs = Direction::vertical;
                    }
                    return std::make_unique<Separator>( runs );
                }
                case NodeType::table: {
                    auto table = std::make_unique<Table>();
                    fill_table( *table, at + 1, end );
                    return table;
                }
                case NodeType::scroll: {
                    auto scroll = std::make_unique<ScrollPane>();
                    if( node.inside != 0 ) {
                        scroll->set_content( make( at + 1 ) ); // check(): one at most
                    }
                    return scroll;
                }
                case NodeType::stack: {
                    auto stack = std::make_unique<Stack>();
                    for( std::size_t inner = at + 1; inner < end; inner = after( inner ) ) {
                        stack->add( make( inner ) );
                    }
                    return stack;
                }
                case NodeType::custom:
                    break;
            }
            std::unique_ptr<Widget> custom;
            if( hooks_.custom != nullptr ) {
                custom = hooks_.custom( hooks_.context, text );
            }
            if( !custom ) {
                built_short( LayoutProblem::unknown_custom, at );
                custom = std::make_unique<Label>( "no custom type " + text );
            }
            return custom;
        }

        /// What a button does when tapped: its line, kept in the button.
        [[nodiscard]] Button::Callback tap_of( std::string_view line ) const
        {
            if( hooks_.tapped == nullptr ) {
                return {};
            }
            return TapCall{ hooks_.tapped, hooks_.context, std::string( line ) };
        }

        const Layout &layout_;
        std::span<const LayoutNode> nodes_;
        const Skin &skin_;
        const LayoutHooks &hooks_;
        LayoutCheck short_{};
};

} // namespace

LayoutCheck instantiate( const Layout &layout, Table &root, const Skin &skin,
                         const LayoutHooks &hooks )
{
    if( const LayoutCheck checked = check( layout ); !checked.ok() ) {
        return checked;
    }
    Builder builder( layout, skin, hooks );
    builder.fill_table( root, 0, layout.nodes().size() );
    return builder.result();
}

} // namespace sq::gui
