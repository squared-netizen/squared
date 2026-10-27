#pragma once

#include <squared/gui/layout.hpp>

#include <memory>
#include <string_view>

namespace sq::gui
{

class List;
class Skin;
class Table;
class Widget;

/**
 * @brief What `instantiate` hands back to its caller (the Stage): taps, bound
 * nodes and custom types. Function pointers and one context, so nothing is
 * allocated to call them. Any can be left out.
 */
struct LayoutHooks final
{
        /** @brief A button was tapped: its line, as the layout wrote it (`read_tap` reads it). */
        void ( *tapped )( void *context, std::string_view line ) noexcept = nullptr;

        /**
         * @brief A node with a `bind` was built, as `widget`: once per such
         * node, while instantiating. `layout.text( node.bind_text )` is the
         * address's name.
         */
        void ( *bound )( void *context, const Layout &layout, const LayoutNode &node,
                         Widget &widget ) noexcept = nullptr;

        /** @brief Make the custom type `name`; nullptr when there is none. */
        std::unique_ptr<Widget> ( *custom )( void *context, std::string_view name ) = nullptr;

        void *context = nullptr;

        /**
         * @brief A button in a list's row was tapped, and its telegram's
         * address takes the row's uid (`do item.use`): its line, without the
         * uid. nullptr: `tapped` hears it, without one.
         */
        void ( *tapped_row )( void *context, std::string_view line ) noexcept = nullptr;

        /**
         * @brief A list was built. Its rows are built later, as it lays out,
         * with the hooks it is given now (`List::set_row_hooks`), or these.
         */
        void ( *listed )( void *context, const Layout &layout, const LayoutNode &node,
                          List &list ) noexcept = nullptr;
};

/**
 * @brief Build a layout's widgets into `root`: the one function that makes
 * widgets from nodes, whichever avenue wrote them (a compiled layout file, or
 * a C++ screen's `LayoutWriter`).
 *
 * The layout is checked first (`check`): a layout that fails builds nothing.
 * Otherwise every node is built, and the result names the first thing that
 * built short (a custom type no hook makes, shown as a label saying so; an
 * image whose drawable the skin lacks). The layout need not outlive this
 * call: a tap's line is copied into its button.
 *
 * Allocates: each widget, its words, and each tap's line, once, here; nothing
 * while it runs. `root` should be empty.
 */
[[nodiscard]] LayoutCheck instantiate( const Layout &layout, Table &root, const Skin &skin,
                                       const LayoutHooks &hooks = {} );

} // namespace sq::gui
