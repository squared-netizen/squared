# Layouts &mdash; internals

Programmer docs: [../programmer/layouts.md](../programmer/layouts.md). The
decision record: the app model's "Screens as data (decided, 7.0)".

Four files in `gui/`: `layout.hpp` (the table, `read_tap`, `check`),
`layout_writer.hpp` (the C++ avenue), `instantiate.hpp` (widgets from
nodes), `layout_file.hpp` (reading `names.json` and layout files), each with
its `src/` file; and the build-time compiler, `tools/sqgen/sqgen.cpp`.
Sprint 7.0a (the table) and 7.0b (files and `sqgen`), 2026-10-21 and 22.

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
| 1 | eight one-bit flags | `grow_x`, `grow_y`, `fill_x`, `fill_y`, `new_row`, `vertical`, `has_numbers`, `sends_set` |
| 1 | `value` | the bound value's `messaging::ValueType`; `none` without a bind |
| 1 | `row_uid` | one bit (7.5b): in a list's row, `bind`'s address or a button's telegram takes the row's uid |
| 1 | padding | to the 8-byte alignment of `bind` |

7.5a added both and the node is still 32 bytes: `sends_set` is the eighth
bit of the flags' byte, and `value` (a one-byte enum, declared after the
bitfields) takes one byte of what was padding. 7.5b's `row_uid` opens a
second byte of flags, after `value`; one byte of padding is left.

**A list node** (7.5b) is a container of one node, its row: `bind` is the
model's list (its rows' name, `inventory`), `value` is `integer` (a row's
uid), and nothing else a value node has. `holds_nodes` counts it; sqgen
writes it as any node.

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

**`Layout`** is a 72-byte view: name, kind, a pane's shape, and spans of
nodes, text and numbers. It owns nothing. The shape (`PaneShape`: a
`PanePlace` and `modal`, 2 bytes, 7.3) sits after the kind in what was tail
padding, so it costs nothing; a screen carries the default and ignores it.
The constructor takes it last, after the numbers, so a screen's definition is
unchanged. `pane_place_name` / `pane_place_named` map a place to its file
word and back, like the node types' pair. The writer carries a shape too
(`set_pane`, kept by `clear` like the name and kind), and gives it to the
layout it writes: a C++ pane's comes from its type's constants that way
([stage.md](stage.md#panes-73)).

## `check`

One pass over the nodes, O(n) time, with a stack of the open containers (16
entries of 16 bytes, on the stack) for O(depth) space. It closes containers
as their end passes (and counts a scroll's direct children there), checks
each node's tree bounds against its container's end, its text offsets
against the pool, `new_row` only in tables, depth at most 16, then the node
itself: a list's bind (its list, by a name its text hashes to, an integer,
nothing more: `check_list`); at most one node in a list, and no list inside
a list's row (`bad_list`); `row_uid` only inside a row, on a bind or a
button whose tap is a telegram; a bind only on the five types that show a value (`shows_value`), with
a name that hashes to the id and a `value` the node can show (`shows_type`:
a label any but uids, a check box a bool, a text field text, a slider or a
progress bar a number or an integer); no `value` and no `sends_set` without a
bind; `sends_set` only on a node that edits (`edits_value`: a check box, a
slider, a text field); a tap only on a button, a line `read_tap`
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
- **Lists** (7.5b): `add_list( rows )` (on the writer or a node) takes a
  list's key (`rows_key`: an integer `state` with a uid) and writes a list
  node; its row is the next node added to it. A key that takes a uid
  compiles in `bind` and `tap` now; the writer refuses it outside a row
  (`in_row`: a list before the node whose nodes reach it, a scan of the
  nodes before it) and sets `row_uid` inside one. A list in a row is refused
  as it is added, a second row as it is added to a list.
- **`bind(key)`** records the key's value type and whether it carries
  `set` (7.5a): `bind_name( name, text, K::type, settable )` refuses a type
  the node cannot show (`shows_type`) and sets `sends_set` only when the
  node edits. The check makes a refused shape impossible, so a layout is
  bound the same way whichever avenue wrote it.
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

## Layout files: `layout_file.cpp`

Build-time code: it runs in `sqgen` (and in a tool such as the editor),
never in an app, so it is on the tools side of the standard-library line and
allocates as it likes.

**JSON** comes from `sq::data` (vendored yyjson, then a `JsonValue` DOM:
objects are a `std::map`, arrays a `std::vector`). Only a syntax error has a
line and column: yyjson's DOM keeps no positions per value. So every other
problem names the **path** to the value (`rows[1][0].bind`), which is as
exact and survives reformatting the file. Two quirks the reader handles: a
positive whole number is stored unsigned (`whole_of` reads either signedness
as `int64_t`), and a duplicate key is refused by the parser already.

**`Reader`** is the file's name, the problem list and an `ok` flag. Reading
never stops at a problem: a bad value adds one and is skipped, so one run
reports all of them. A layout's kind is the exception, read on its own
first: navigation is checked against every file's kind, so `sqgen` reads all
the kinds before any layout. A pane's `place` and `modal` (7.3) are read
with the file's own keys, before its rows, into the writer's shape; on a
screen either is a problem ("a pane's: this is a screen"), as is a place
that is not one of the five or a `modal` that is not a bool.

**`names.json`** becomes a `NameSchema`: entries sorted by name, so `find`
is a binary search and a duplicate shows as two neighbours. `find_id` finds
by the hashed `Name` (a linear scan: tens of entries), which is what a
telegram read from a tap carries. A list `{ "list": "item" }` becomes two
entries, its rows' key (`int`, `state`, uid) and `<name>.count`, as decided
for 7.5's lists.

**`LayoutReader`** walks the rows, writing each node through the same
`LayoutWriter` the C++ avenue uses, so both avenues make the same table by
construction and `check` applies to both. Two things the typed API refuses
at compile time a file needs at run time: adding a node by its `NodeType`
(not a widget type) and binding by a name (not a `Key`).
`detail::FileWriting`, a friend of `LayoutWriter` and `NodeWriter`, does
both; it is not for apps (it is in `detail`, and its definition is in
`layout_file.cpp`, so nothing else can call it). Each node's keys are
checked against a per-type list (`takes_key`); a bind against the schema
(present, carries `state`, a type the node can show: `shows_type`, the same
function the check uses; one that takes a uid only in a list's row, and
then only its element's, `item.*` in a list of `item`), and the entry's type,
its `set` verb and its uid go to the node as the C++ avenue's key would give
them; a list node's `bind` names a list (an entry made by `{ "list": ... }`)
and its `row` is read with that list as the row's; a
tap by kind: a telegram is parsed with `messaging::read_telegram` into 4 KB
on the stack and checked for address, verb, uid and value type, then stored
as written; a navigation against the targets and the schema's `screens`.

## `sqgen`

One program, built for the build machine. Pass by pass:

1. **Arguments.** Usage problems exit 2.
2. **`names.json`** is read (`read_names`), then each name becomes an
   identifier: `.` to `_`, refused when it starts with a digit or `_`,
   holds `__`, is a keyword (C++20's and the alternative tokens) or a name
   the generated code uses (`sq`, `names`, `layouts`), or another name
   became it too.
3. **Layout files**, sorted by name: each one's name (the file's stem, a
   valid name, a valid identifier, not shared) and kind, then each
   compiled through `read_layout` into a writer over the most a `Layout`
   can hold (65,533 nodes, 65,535 bytes of text and 65,535 numbers: about
   2.4 MB of scratch, allocated once per run and reused), and checked.
4. **Any problem**: each printed, `N problems; nothing written`, exit 1.
5. **Writing**: each file's text is made in memory and written only when it
   differs from what is there, through a `.part` file and a rename, so a
   build interrupted midway leaves either the old file or the new one.

What it writes:

- `names.hpp`: `inline constexpr sq::messaging::Key<T, verbs[, with_uid]>
  id{ "name" };` per entry, its note as the comment. A `constexpr` key is
  its name's hash, so an unused key costs nothing in the app.
- `layouts.hpp`: `extern const sq::gui::Layout name;` per layout, `all()`,
  `find()`, and `screens()` (7.2): `names.json`'s `screens`, sorted, which
  the Stage checks the app added ([stage.md](stage.md#c-screens-72)).
- `layouts.cpp`: per layout, in an anonymous namespace, its text as one
  `char` array (escaped: `\"`, `\\`, and octal for `'\0'`, `?` and anything
  unprintable, so no trigraph or digit after an escape can change it), its
  nodes as designated initializers (`.bind` through the `constexpr`
  `Name( "..." )`, so the hash is the same function's), its numbers; then a
  `constinit const Layout` over them (a pane's with its shape last:
  `{}` for numbers it has none of, then `sq::gui::PaneShape{
  sq::gui::PanePlace::bottom, true }`), and a sorted array of pointers that
  `find` binary-searches; the screens as a `constexpr` array of
  `string_view`s.

**Why it compiles from the library and not a copy.** `sqgen` links
`layout.cpp`, `layout_writer.cpp` and `layout_file.cpp` themselves: the
tables it writes are the writer's, checked by the same `check`, and
`sqgen_test` compares one with a C++-written one field by field.

## The app build: `mk/pkg_squared_core.mk`

The package's mk file, not the project template, so no template change. It
applies only when `sq_app/names.json` or `sq_app/layouts/*.json` exists:

- `$(BUILD)/host/sqgen` is built with `HOST_CXX` from 11 framework sources
  plus `yyjson.c` (`HOST_CC`), like `sqconsole`, and rebuilt when one of
  them changes, or a layout, messaging or data header.
- `$(BUILD)/gen/layouts.cpp` depends on sqgen, every input file, and an
  `inputs` file listing them, rewritten only when the list changes (a
  `$(shell)` at parse time), so adding or removing a layout regenerates.
  The recipe touches it, as sqgen may leave it unchanged.
- It joins the app through the existing hooks: `SQ_KIT_SRC +=` the
  generated source, `SQ_KIT_CPPFLAGS += -I$(BUILD)/gen`.
- Every app object gets an **order-only** prerequisite on it: generation
  happens before any compile, and the compiler's dependency files then say
  which objects really include the generated headers.

## Memory

In the app: the tables only, in read-only data. Per layout: 32 bytes a
node, its text (each word and line once, with a `'\0'`), 4 bytes a number,
the 72-byte `Layout`, and 8 bytes in the sorted array. The test's
`settings` (15 nodes, 212 bytes of text, 5 numbers) is 792 bytes. Nothing is allocated
until `instantiate`. `names.hpp` adds nothing that is not used. No JSON
reader and no `sqgen` is in the app: `layout_file.cpp` is in
`libsquared_gui.a`, but a static archive's member is linked only when
something refers to it, and nothing in an app does.

## Standard library

Kept, in the push path only: `std::unique_ptr` and `std::string` (the
widgets' own types) and `std::function` (inside `Button`, which holds the
`TapCall`). Passed over: `std::function` for the hooks (function pointers).
See [standard-library-deviations.md](standard-library-deviations.md).

Build-time code (`layout_file.cpp`, `sqgen`) uses the standard library
freely, `std::map` (in `JsonValue`), `std::string` paths and messages,
`std::vector` problems, `<filesystem>`: none of it reaches an app.

## Tested

`gui/test/screen_layout_test.cpp` (`screen_layout_test`; console's own
`layout_test` is about its text):

- **`read_tap`:** every kind, and seven lines it refuses.
- **Pane shapes:** every place's name read back, a word that is not one, the
  writer's shape reaching its layout and surviving `clear`.
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

`gui/test/layout_file_test.cpp` and `layout_file_cases.txt`: the reader.
The cases are data, one a line (kind, JSON, the problem expected: exactly,
by its start, `ok`, or a count), so a case reads as the file it is about and
no C++ formatter touches the long JSON. Every case runs and every failing one
is printed with its line. The file is a prerequisite of the test binary,
because a test runs again only when its binary is newer than its pass.
Beside the cases: `names.json` read into its schema, and a file giving the
same table as the C++ writer.

`gui/test/sqgen_test.cpp`: `sqgen` end to end. The gui Makefile builds it,
runs it on `test/sqgen/`, and compiles the test with what it wrote: the keys'
types (`static_assert`), the layouts found, sorted and checked, the pane
`discard`'s place and modal from its file, `settings` equal to the writer's
table, and built. Then it runs `sqgen` again: unchanged
input writes nothing; the wrong files in `test/sqgen/bad/` and `keyword/`
print exactly their `expected.txt` and write nothing; a missing file or
directory, and bad arguments.

Planted bugs for 7.0b, 29, each fail a test (`plant71.py`).

**Lists (7.5b):** the checker's list cases (a row, none, two; a list's
bind: none, text, a field's name, a set; a list in a row; a row's uid in a
row, outside one, on a navigation, on a separator), the writer's list and
its refusals (a uid outside a row, before or after the list; a list in a
row; a second row), `instantiate` making a `List` that copied its row and
calling `listed`, the fuzz writing lists and uid keys (most refused, every
layout still checking); ten new file cases (a list without its bind or
row, a bind that is no list, a key a list does not take, a uid of another
element, a uid written in the line, a list in a row, a type the row's node
cannot show) and one accepted; a list from a file giving the C++ writer's
table; sqgen's `settings` ending in a list, equal to the writer's. Their
planted bugs are with the Stage's ([stage.md](stage.md#tested)).

Not covered: the cell placement itself (grow, pad, span reach `Cell`, which
no test reads back without laying out with a painter); the Stage's tests will
see it.
