# Layouts &mdash; internals

Programmer docs: [../programmer/layouts.md](../programmer/layouts.md). The
decision record: the app model's "Screens as data (decided, 7.0)".

Three files in `gui/`: `layout.hpp` (the table, `read_tap`, `check`),
`layout_writer.hpp` (the C++ avenue), `instantiate.hpp` (widgets from
nodes), each with its `src/` file. Sprint 7.0, 2026-10-21.

## Why a table

A screen is data so the visual editor can read and write it. A layout file
compiles to a table at build time, so a release app carries no layout parser
and makes nothing until a screen is pushed (priority 1). The other avenue,
C++, writes the same table, so there is one thing to build widgets from, one
function that builds them (`instantiate`), and one checker (`check`) both
avenues pass through. Rejected: generating builder code (`root.add<...>()`
written out by the tool), which would have needed a second interpreter for
dev-time reloading that must agree with the generator.

## `LayoutNode`: 32 bytes

| Bytes | Field | |
|---:|---|---|
| 8 | `bind` | the bound address's name id; 0: none |
| 2 &times; 6 | `text`, `tap`, `bind_text`, `style`, `inside`, `numbers` | offsets into the text and numbers, and the subtree's size |
| 4 | `type`, `align_x`, `align_y`, `span` | one byte each |
| 4 | `pad` | below 0: the cell's own |
| 1 | seven one-bit flags | `grow_x`, `grow_y`, `fill_x`, `fill_y`, `new_row`, `vertical`, `has_numbers` |
| 3 | padding | to the 8-byte alignment of `bind` |

`static_assert`ed. A trivially copyable aggregate, so the build writes it
with designated initializers into `.rodata`; defaulted `==` (the writer's test
compares its output with a hand-written table field by field).

**Document order.** A container is followed by its nodes, and `inside` counts
them at every depth. The next sibling of node `i` is `i + 1 + inside`;
children are walked by hopping, so no parent or child indices are stored.
16-bit fields cap a layout at 65,533 nodes and 65,535 bytes of text: a
screen has tens.

**Text** is a pool of words, each ended by `'\0'`, starting with a `'\0'` of
its own so offset 0 means none. `Layout::text(offset)` is constexpr: a
compiled layout's words can be read in a `static_assert`. **Numbers** are a
separate `float` pool, used by sliders (minimum, maximum, step) and progress
bars (minimum, maximum) only, so other nodes pay nothing for them.

**`Layout`** is a 72-byte view: name, kind, and spans of nodes, text and
numbers. It owns nothing.

## `check`

One pass over the nodes, O(n) time, with a stack of the open containers (16
entries of 16 bytes, on the stack) for O(depth) space. It closes containers
as their end passes (and counts a scroll's direct children there), checks
each node's tree bounds against its container's end, its text offsets
against the pool, `new_row` only in tables, depth at most 16, then the node
itself: a bind only on the five types that show a value (`shows_value`), with
a name that hashes to the id; a tap only on a button, a line `read_tap`
accepts; numbers in range, minimum below maximum, a slider's step not
negative (written `!( a < b )` so a NaN fails); a custom node's name.
`instantiate` runs it first, so the builder can trust the tree: a bad layout
builds nothing.

**An empty scroll is allowed.** A `ScrollPane` with no content is harmless,
and the writer could not refuse one as it goes (it cannot know no node is
coming), so refusing it would have made a writer's output fail its own check.

## `LayoutWriter`

192 bytes: three spans, the name, a stack of 16 open containers (node,
children so far, a pending row), and counters. Allocates nothing (the test
writes a 5-node layout 100 times under the frame-allocation counter).

- **`add_to(parent)`** finds the parent on the open stack (O(depth)), closes
  what was opened after it, adds the node at the end, and increments
  `inside` on every container still open: they are all its ancestors. A
  parent not on the stack is finished; the node is not written. A container
  past depth 16 is not written either, so the open stack never overflows.
- **Refusals** set one flag, `complete_`, and change nothing else, so the
  table is always consistent: the fuzz writes 2,000 random layouts, each
  checks and builds, most with something refused along the way.
- **Rows.** `row()` marks the next node of that table; `new_row` is set only
  if the table already has a node, since `Table::row()` on an empty table
  does nothing anyway and a compiled layout should not differ for it.
- **Typed taps** write the telegram's canonical line through
  `messaging::write_telegram` (the one writer of the text form), with a name
  table of one entry on the stack (272 bytes) so the line names the key. A
  text value's `DataRef` is pointed at the whole data first, as the Telegraph
  does when it copies text into its ring: `detail::pack` leaves it empty.
  `detail::pack` moved from `telegraph.hpp` to `messaging/src/pack.hpp` for
  this, so the writer need not include the whole Telegraph.
- **The constraints** are named concepts in `gui::detail` (`shown_key`,
  `done_key`, `sent_value`): a bind takes a key that carries `state` and no
  uid; a tap a key that carries `do` (or `set`, with a value). Short named
  concepts also keep each `requires` on one line, which clang-format 18 and
  21 lay out the same.

## `instantiate`

A `Builder` walks the table recursively (depth at most 16, from `check`):
`make` builds a node's widget, `fill_table` adds each child of a table to its
cell with the node's placement, a scroll gets its one node as content, a
stack its nodes in order. A factory over a type tag: an `enum` and a
`switch`, chosen over a registry or an abstract factory (priority 4 would buy
nothing: the types are fixed, and custom ones come through one hook).

**Allocation, at push time only:** each widget (`std::make_unique`, as today's
`gui` owns its children), its words (`std::string`, as the widgets take
them), and each tap: a `TapCall` (hook, context, `std::string` line) in the
button's `std::function`. The line is copied because a C++ screen's layout is
scratch memory, dropped once its widgets are built. Nothing per frame. The
decision of 7.0 was to build today's widgets now; a single arena per screen,
sized from its nodes, can replace this behind `instantiate` later without
changing any app.

**Hooks** are function pointers and one context (`LayoutHooks`, 32 bytes),
not `std::function`: nothing allocated to hold them, no type erasure.

**Building short.** A custom type no hook makes is replaced by a label
saying so, and an image whose drawable the skin lacks is built empty; the
first such node is returned, and everything else is still built, so a
missing piece shows on the screen rather than leaving it blank.

## Standard library

Kept, in the push path only: `std::unique_ptr` and `std::string` (the
widgets' own types) and `std::function` (inside `Button`, which holds the
`TapCall`). Passed over: `std::function` for the hooks (function pointers).
See [standard-library-deviations.md](standard-library-deviations.md).

## Tested

`gui/test/screen_layout_test.cpp` (`screen_layout_test`; console's own
`layout_test` is about its text):

- **`read_tap`:** every kind, and seven lines it refuses.
- **`check`:** every problem, from hand-made tables.
- **The two avenues meet:** the writer's table for `settings` equals, field
  by field and in its text, the constant table `sqgen` will write.
- **Typed taps:** `set`, `do` with and without a value, text with quotes.
- **The writer:** each refusal; storage running out, including text filled
  to its last byte.
- **`instantiate`:** a compiled layout, and every kind of node from the
  writer, with the scratch cleared before the taps fire.
- **The fuzz:** 2,000 random layouts.

Planted bugs, 19 here and 2 in the bridge, each fail a test (the
`plant70.py` script of the package's checks).

Not covered: the cell placement itself (grow, pad, span reach `Cell`, which
no test reads back without laying out with a painter); the Stage's tests will
see it.
