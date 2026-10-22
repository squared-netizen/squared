# Layouts

Internals: [../developer/layouts.md](../developer/layouts.md). Part of the
[gui](gui.md) module.

A **layout** is a screen or a pane as data: a table of nodes, each one a
widget, where it sits in its cell, what it shows and what a tap sends.
There are two ways to make one, and both make the same thing:

- **A layout file**, `sq_app/layouts/<name>.json`, which the build compiles
  into a constant table (the `sqgen` tool: the next package). It is also what
  the visual editor will read and write.
- **C++**: a screen's `build()` writes the same table through a
  `LayoutWriter`.

Either way, one function, `instantiate`, builds the widgets. Until the Stage
arrives (Sprint 7.1), you call it yourself.

| Header | Gives you |
|---|---|
| `squared/gui/layout.hpp` | `Layout`, `LayoutNode`, `NodeType`, `LayoutKind`, `read_tap`, `check` |
| `squared/gui/layout_writer.hpp` | `LayoutWriter`, `NodeWriter`, `Navigate` |
| `squared/gui/instantiate.hpp` | `instantiate`, `LayoutHooks` |

All three come with `squared/gui/gui.hpp`.

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

Once the Stage exists, the first four lines are its job, and the last two
happen when the screen is pushed: a screen keeps only the `add` lines.

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
| `text` | The words it shows; an image's drawable name; a custom node's type |
| `grow`, `grow_x`, `grow_y` | Take the cell's spare room |
| `fill`, `fill_x`, `fill_y` | Fill the cell |
| `pad` | Space around it in its cell |
| `align` | `"start"`, `"center"` or `"end"`, or two of them: across, then down |
| `span` | Columns the cell takes |
| `bind` | An address from `names.json`: the node shows its `state`, and a node that edits sends `set` |
| `tap` | A button's line: a telegram (`do file.open "a.txt"`) or a navigation (below) |
| `style` | A style from the skin, in place of `default` |
| `min`, `max`, `step` | A slider's range and step; a progress bar's range |
| `vertical` | A separator that runs up and down |
| `note` | Words for people; ignored |

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
| `custom` | your type | `text`: the name you register it by |

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
| `layout()` | What was written, as a `Layout` pointing into your storage |
| `complete()` | False when something was not written: storage ran out, or it went where it cannot (below) |
| `clear()` | Start again in the same storage |

`add` returns a `NodeWriter`; every call on it returns it again, so they
chain:

| `NodeWriter` call | Does |
|---|---|
| `grow()`, `grow_x()`, `grow_y()`, `fill()`, `fill_x()`, `fill_y()` | The cell's room |
| `pad(space)`, `align(x, y = center)`, `span(columns)` | The cell's spacing, alignment and columns |
| `style(name)` | A style from the skin |
| `vertical()` | A separator that runs up and down |
| `range(minimum, maximum)`, `step(step)` | A slider's or progress bar's numbers; `step` is a slider's |
| `bind(key)` | Show the key's `state`. Only keys that carry `state` and take no uid compile |
| `tap(line)` | A button's line, as a file writes it |
| `tap(Navigate::push, "settings")` | A navigation: `push`, `replace`, `back`, `open`, `close`, `quit` |
| `tap(key)`, `tap(key, value)` | A telegram: `do <key>`, or `do`/`set <key> <value>`; the type is checked when it compiles |
| `add<W>(text)`, `add_custom(name)` | A node inside this table, scroll (one only) or stack |
| `row()` | In this table, the next node starts a new row |
| `written()` | Whether this node was written |

**Nodes go in document order.** A table, scroll or stack takes nodes until a
node is added to one outside it; adding to it after that is a mistake, and the
node is not written. So write each container's contents before moving on.

**What the writer refuses** (the call is ignored and `complete()` is false):
a node added to one that is finished or holds nothing; a second node in a
scroll; containers deeper than 16; a `bind` on a node that shows no value; a
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
| `bad_bind` | A bind on a node that shows no value, or a name its text does not hash to |
| `bad_tap` | A tap on a node that is not a button, or not a tap's line |
| `bad_numbers` | Numbers missing, on a node that takes none, or a minimum not below its maximum |
| `no_custom_name` | A custom node without its type's name |

## The table itself

You will rarely touch it: the build and the writer make it. A `Layout` is a
view of `LayoutNode`s (32 bytes each), their text and numbers, a `name` and a
`kind` (`screen` or `pane`). `layout.text(offset)` gives the words at an
offset. The details are in the developer page.

## Lua

Not bound: the Lua layer is not built yet.
