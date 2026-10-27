# Layouts

Internals: [../developer/layouts.md](../developer/layouts.md). Part of the
[gui](gui.md) module.

A **layout** is a screen or a pane as data: a table of nodes, each one a
widget, where it sits in its cell, what it shows and what a tap sends.
There are two ways to make one, and both make the same thing:

- **A layout file**, `sq_app/layouts/<name>.json`, which the build compiles
  into a constant table with `sqgen` ([below](#sqgen-the-build-step)). It is
  also what the visual editor will read and write.
- **C++**: a screen's `build()` writes the same table through a
  `LayoutWriter`.

Either way, one function, `instantiate`, builds the widgets. The Stage calls
it when a screen is shown (`app.stage.push( layouts::settings )`,
[stage.md](stage.md)); a program without the App calls it itself.

| Header | Gives you |
|---|---|
| `squared/gui/layout.hpp` | `Layout`, `LayoutNode`, `NodeType`, `LayoutKind`, `read_tap`, `check` |
| `squared/gui/layout_writer.hpp` | `LayoutWriter`, `NodeWriter`, `Navigate` |
| `squared/gui/instantiate.hpp` | `instantiate`, `LayoutHooks` |
| `squared/gui/layout_file.hpp` | Reading `names.json` and layout files yourself: what `sqgen` uses |

The first three come with `squared/gui/gui.hpp`. `layout_file.hpp` does not:
it brings in the JSON reader, which an app does not carry (a release app
reads no layout file; `sqgen` did that when it was built).

## The shortest call site

```cpp
std::array<sq::gui::LayoutNode, 32> nodes;
std::array<char, 512> text;
std::array<float, 16> numbers;
sq::gui::LayoutWriter root( nodes, text, numbers, "settings" );
root.add<sq::gui::CheckBox>( "Sound" ).bind( names::settings_sound );
root.row();
root.add<sq::gui::Button>( "Back" ).tap( sq::gui::Navigate::back );

sq::gui::Table table;
const sq::gui::LayoutCheck built = sq::gui::instantiate( root.layout(), table, skin, hooks );
```

With the Stage, the last two lines happen when the screen is pushed; once C++
screens come (7.2), the first four are its job too, and a screen keeps only
the `add` lines.

## A layout file

```json
{
  "kind": "screen",
  "rows": [
    [ { "type": "label", "text": "Hello" } ],
    [ { "type": "check_box", "text": "Sound", "bind": "settings.sound" } ],
    [ { "type": "button", "text": "Back", "tap": "back", "grow": true } ]
  ]
}
```

A screen or pane is a table: `rows`, each an array of cells, each cell one
node. A nested `table` node has `rows` of its own; a `scroll` holds one node
(`"node": {...}`) and a `stack` a list of them (`"nodes": [...]`).

| Key | Means |
|---|---|
| `type` | What the node builds; see the table below |
| `text` | The words it shows; an image's drawable name |
| `name` | A custom node's type: the name you register it by |
| `grow`, `grow_x`, `grow_y` | Take the cell's spare room |
| `fill`, `fill_x`, `fill_y` | Fill the cell |
| `pad` | Space around it in its cell |
| `align` | `"start"`, `"center"` or `"end"` across, or two of them: across, then down |
| `span` | Columns the cell takes |
| `bind` | An address from `names.json`: the node shows its `state`, and a node that edits (check box, slider, text field) sends `set` when the address carries it ([stage.md](stage.md#bound-values)) |
| `tap` | A button's line: a telegram (`do file.open "a.txt"`) or a navigation (below) |
| `style` | A style from the skin, in place of `default` |
| `min`, `max`, `step` | A slider's range and step; a progress bar's range |
| `vertical` | A separator that runs up and down |
| `note` | Words for people; ignored, here and in every object |

A file's own keys are `kind` (`"screen"`, the default, or `"pane"`), `rows`
and `note`; a pane's also `place` and `modal`:

| Key | Means |
|---|---|
| `place` | Where the pane sits: `"center"` (the default), `"left"`, `"right"` (the screen's height), `"top"`, `"bottom"` (its width) |
| `modal` | `true`: the screen beneath dims and takes no taps until the pane closes. Default `false` |

```json
{ "kind": "pane", "place": "bottom", "modal": true, "rows": [
  [ { "type": "label", "text": "Discard?" } ],
  [ { "type": "button", "text": "No", "tap": "close" } ] ] }
```

How panes open, close and show: [stage.md](stage.md#panes).

A key a node or file does not take is a mistake, not ignored:
`"grwo": true` is reported, so a typo cannot silently do nothing. A layout's
name in the build is its file's: `settings.json` is the layout `settings`
(lowercase words, digits and `_`).

| `type` | Widget | Takes |
|---|---|---|
| `label` | `Label` | `text`, `bind` |
| `button` | `Button` | `text`, `tap` |
| `check_box` | `CheckBox` | `text`, `bind` |
| `text_field` | `TextField` | `text`, `bind` |
| `slider` | `Slider` | `min`, `max`, `step`, `bind` |
| `progress_bar` | `ProgressBar` | `min`, `max`, `bind` |
| `image` | `Image` | `text`: a drawable in the skin |
| `separator` | `Separator` | `vertical` |
| `table` | `Table` | `rows` |
| `scroll` | `ScrollPane` | one node |
| `stack` | `Stack` | nodes |
| `custom` | your type | `name`: the name you register it by |

## Taps

A tap is one line, in the same text the dev log writes, so a layout, the log
and a replay file all read alike.

| Line | Does |
|---|---|
| `set <address> <value>` | Sends `set` to the model |
| `do <address> [value]` | Sends `do` to the model |
| `push <name>` | That screen, over this one |
| `replace <name>` | That screen, instead of this one |
| `back` | What the back button does |
| `open <name>` | That pane, over this screen |
| `close` | The pane the button is in |
| `quit` | The app (through its quit path) |

`read_tap(line)` reads one: a `Tap` with its `kind` and `target` (the name
navigated to, or the whole telegram line). `TapKind::invalid` for anything
else.

In a file, a tap is also checked against the app: a telegram's address must
be in `names.json`, carry that verb, and take that type of value (a `number`
is written with a point, `11.0`; `11` is an `int`). A `push` or `replace`
must name a screen, an `open` a pane: another layout file, or a C++ screen
listed in `names.json`'s `screens`. `close` belongs in a pane.

## `names.json`

Every address the app's model and screens use, in one file,
`sq_app/names.json`. `sqgen` makes each one a typed key, and checks every
`bind` and `tap` in the layout files against it.

```json
{
  "names": {
    "settings.volume": { "type": "number", "verbs": ["state", "set"] },
    "file.open":       { "type": "text",   "verbs": ["do"] },
    "game.over":       {                   "verbs": ["event"] },
    "item.name":       { "type": "text",   "verbs": ["state"], "uid": true },
    "inventory":       { "list": "item", "note": "What the player carries" }
  },
  "screens": ["workspace"]
}
```

| Key of an address | Means |
|---|---|
| `type` | Its value: `bool`, `int`, `number`, `vec2`, `vec3`, `vec4`, `text`, `uids`; none for a bare `do` or `event` |
| `verbs` | Which of `state`, `event`, `set`, `do` it carries; at least one. `state` and `set` need a `type` |
| `uid` | `true`: it is one of many, told apart by a uid (a row of a list) |
| `note` | Words for people; becomes the key's comment |
| `list` | Instead of the above: a list whose rows are `<list>.*` addresses. It declares `<name>` (the rows: an `int` `state` with a uid) and `<name>.count` |

`screens` lists the screens and panes written in C++, so a layout may
navigate to them. A name is lowercase words, digits and `_` between dots.

The keys come out in `names.hpp`, a dot becoming `_`:

| `type` | Key's value type |
|---|---|
| none | `void` |
| `bool`, `int`, `number` | `bool`, `std::int64_t`, `double` |
| `vec2`, `vec3`, `vec4` | `std::array<float, 2>` (3, 4) |
| `text` | `std::string_view` |
| `uids` | `std::span<const std::uint32_t>` |

```cpp
#include <names.hpp>

ui.set( names::settings_volume, 7.0 );           // a Key<double, state | set>; ui is the TelegraphUi
root.add<sq::gui::Slider>().bind( names::settings_volume );
```

## sqgen: the build step

An app with `sq_app/names.json` or any `sq_app/layouts/*.json` has them
compiled before its own sources, into `build/gen/`. You do nothing to turn
it on; an app with neither builds as before.

| File | Holds |
|---|---|
| `names.hpp` | Every address as a typed key, in `namespace names` |
| `layouts.hpp` | Every layout, declared (`extern const sq::gui::Layout settings;`), in `namespace layouts`, with `all()` and `find(name)`; and `screens()`, the C++ screens `names.json` lists, which the Stage checks were added |
| `layouts.cpp` | Their tables, as constants in read-only data; compiled into the app |

```cpp
#include <layouts.hpp>

const sq::gui::LayoutCheck built = sq::gui::instantiate( layouts::settings, table, skin, hooks );
const sq::gui::Layout *named = layouts::find( "settings" );   // nullptr: none by that name
```

The build prints one line, `sqgen  3 keys, 2 layouts; 1 file changed`. A
file is written only when its text changes, so editing one layout rebuilds
only `layouts.cpp` and what includes `layouts.hpp`, not every source that
includes `names.hpp`.

**Problems stop the build.** Every file is read and checked before anything
is written. Each problem is one line, its file first, then where in it:
a line and column for a file that is not JSON, otherwise the path to the
value:

```
sqgen: names.json: names["settings.Volume"]: not a name: lowercase words, digits and _, between dots
sqgen: settings.json:4:16: not JSON: unexpected character, expected ':' after key
sqgen: landing.json: rows[1][0].bind: settings.volme is not in names.json
sqgen: landing.json: rows[2][0].tap: settings.volume takes a number, not an int: write a number with a point (7.0)
sqgen: 4 problems; nothing written
```

A name that cannot be a C++ name is reported too: one that becomes a keyword
(`new`), starts with a digit or `_`, or that two names share
(`a_b.c` and `a.b_c` are both `a_b_c`).

Run by hand (the build runs it for you):

```sh
sqgen [--names FILE] [--layouts DIR] --out DIR
```

| Exit | Means |
|---|---|
| 0 | Written, or already current |
| 1 | Problems, each printed; nothing written. Also a `--names` file that cannot be read, or a `--layouts` that is not a directory |
| 2 | Wrong arguments: no `--out`, an option without its value, or one sqgen does not know |

Without `--names`, a layout's binds and telegram taps have no address to be
checked against, so each one is a problem. Without `--layouts`,
`layouts.hpp` declares none.

## Reading files yourself: `layout_file.hpp`

What `sqgen` is made of, for a tool of your own (an editor, a checker).
Every problem is added to a `FileProblems` (a list of `{where, what}`) and
reading goes on, so one pass reports them all.

| Function | Does |
|---|---|
| `read_json(file, text, out, problems)` | Parse `text` as the file named `file` into `out`; false when it is not JSON |
| `read_names(file, root, schema, problems)` | `names.json` into a `NameSchema`: its `names` (sorted; `find(name)`, `find_id(id)`) and `screens` (`has_screen(name)`) |
| `read_layout_kind(file, root, problems)` | The file's `kind`, alone: screen when it is left out or wrong (and then a problem) |
| `read_layout(file, root, schema, targets, out, problems)` | The whole file into a `LayoutWriter`, checked against the schema and `targets`, the other layouts (name and kind) |

Each returns false (or adds a problem) when the file was wrong; a writer a
wrong file was read into is not to be used. Allocates freely: these are for
tools, not for an app.

## From C++: `LayoutWriter`

The writer writes into storage you give it (nodes, text, numbers) and
allocates nothing. Words are copied into the text storage, so a temporary
`std::string` is fine.

| Call | Does |
|---|---|
| `LayoutWriter(nodes, text, numbers, name = {}, kind = screen)` | A writer over your storage |
| `add<W>(text)` | Add a widget to the layout's own table: `Label`, `Button`, `CheckBox`, `TextField`, `Slider`, `ProgressBar`, `Image`, `Separator`, `Table`, `ScrollPane`, `Stack`. Another type does not compile |
| `add_custom(name)` | Add a custom node |
| `row()` | The next node added to the layout's own table starts a new row |
| `set_pane(shape)` | How it shows as a pane: `{ PanePlace::bottom, true }`, a file's `place` and `modal` |
| `layout()` | What was written, as a `Layout` pointing into your storage |
| `complete()` | False when something was not written: storage ran out, or it went where it cannot (below) |
| `clear()` | Start again in the same storage, with the same name, kind and pane shape |

`add` returns a `NodeWriter`; every call on it returns it again, so they
chain:

| `NodeWriter` call | Does |
|---|---|
| `grow()`, `grow_x()`, `grow_y()`, `fill()`, `fill_x()`, `fill_y()` | The cell's room |
| `pad(space)`, `align(x, y = center)`, `span(columns)` | The cell's spacing, alignment and columns |
| `style(name)` | A style from the skin |
| `vertical()` | A separator that runs up and down |
| `range(minimum, maximum)`, `step(step)` | A slider's or progress bar's numbers; `step` is a slider's |
| `bind(key)` | Show the key's `state`, and send `set` from a node that edits when the key carries it. Only keys that carry `state` and take no uid compile; a value type the node cannot show is refused (a check box takes a bool, a text field text, a slider or progress bar a number or an integer, a label any but a uid list) |
| `tap(line)` | A button's line, as a file writes it |
| `tap(Navigate::push, "settings")` | A navigation: `push`, `replace`, `back`, `open`, `close`, `quit` |
| `tap<About>(Navigate::push)` | A navigation to the C++ screen `About`, by its `name`, checked when it compiles ([stage.md](stage.md#c-screens)) |
| `tap(key)`, `tap(key, value)` | A telegram: `do <key>`, or `do`/`set <key> <value>`; the type is checked when it compiles |
| `add<W>(text)`, `add_custom(name)` | A node inside this table, scroll (one only) or stack |
| `row()` | In this table, the next node starts a new row |
| `written()` | Whether this node was written |

**Nodes go in document order.** A table, scroll or stack takes nodes until a
node is added to one outside it; adding to it after that is a mistake, and the
node is not written. So write each container's contents before moving on.

**What the writer refuses** (the call is ignored and `complete()` is false):
a node added to one that is finished or holds nothing; a second node in a
scroll; containers deeper than 16; a `bind` on a node that shows no value,
or of a type it cannot show; a
`tap` on anything but a button, or a line `read_tap` refuses; `range` or
`step` on a node without numbers, or a range whose minimum is not below its
maximum; `vertical` on anything but a separator; running out of storage.
Whatever was written is still a layout that passes `check`.

## Building widgets: `instantiate`

```cpp
[[nodiscard]] LayoutCheck instantiate( const Layout &layout, Table &root, const Skin &skin,
                                       const LayoutHooks &hooks = {} );
```

It checks the layout first; one that fails builds nothing, and the result
says why. Otherwise it builds every node into `root` (which should be empty)
and returns the first thing that built short, still building the rest:

| Result | Means |
|---|---|
| `ok()` | Everything built |
| `unknown_custom` | No hook made that custom type: a label saying so stands in |
| `unknown_drawable` | An image's drawable is not in the skin: it shows nothing |

The layout need not outlive the call: a tap's line is copied into its
button. Allocates: each widget, its words, and each tap's line, once; nothing
afterwards.

`LayoutHooks` is how the result reaches you. Any of them can be left out:

| Hook | Called |
|---|---|
| `tapped(context, line)` | When a built button is tapped, with its line |
| `bound(context, layout, node, widget)` | Once per node with a `bind`, as it is built: `layout.text(node.bind_text)` is the address |
| `custom(context, name)` | For each custom node: return the widget, or nullptr |
| `context` | Passed to each |

## `check`

`check(layout)` says whether a layout can be built: `LayoutCheck` holds the
first `problem` and its `node` index; `ok()` when there is none.
`problem_text(problem)` says it in words.

| `LayoutProblem` | Means |
|---|---|
| `bad_text` | The text does not start and end with `'\0'`, or an offset is outside it |
| `bad_tree` | A node's `inside` runs past its container's end |
| `too_deep` | Containers nested deeper than 16 |
| `not_a_container` | Nodes inside one that is not a table, scroll or stack |
| `scroll_holds_one` | A scroll with more than one node |
| `misplaced_row` | A new row outside a table |
| `bad_type` | Not a `NodeType` |
| `bad_bind` | A bind on a node that shows no value, a name its text does not hash to, a value type the node cannot show, or a `set` on a node that edits nothing |
| `bad_tap` | A tap on a node that is not a button, or not a tap's line |
| `bad_numbers` | Numbers missing, on a node that takes none, or a minimum not below its maximum |
| `no_custom_name` | A custom node without its type's name |

## The table itself

You will rarely touch it: the build and the writer make it. A `Layout` is a
view of `LayoutNode`s (32 bytes each), their text and numbers, a `name`, a
`kind` (`screen` or `pane`) and `pane()`, a `PaneShape` (`place`, a
`PanePlace`, and `modal`; a screen's is the default and ignored).
`layout.text(offset)` gives the words at an offset. `pane_place_name` and
`pane_place_named` turn a place into its file word and back. The details are
in the developer page.

## Lua

Not bound: the Lua layer is not built yet.
