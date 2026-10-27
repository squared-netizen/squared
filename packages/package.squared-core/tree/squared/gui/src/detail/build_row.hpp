#pragma once

// Internal to the squared build: how a List builds its rows. Not installed
// and not part of the public API.

#include <squared/gui/instantiate.hpp>
#include <squared/gui/layout.hpp>

namespace sq::gui
{
class Skin;
class Table;
} // namespace sq::gui

namespace sq::gui::detail
{

/**
 * @brief Build `row`, a list's row as a layout, into `into`, with `hooks`:
 * `instantiate` without its check, which the layout the row came from has
 * passed already (a row on its own would fail it: its uids need a list).
 */
[[nodiscard]] LayoutCheck build_row( const Layout &row, Table &into, const Skin &skin,
                                     const LayoutHooks &hooks );

} // namespace sq::gui::detail
