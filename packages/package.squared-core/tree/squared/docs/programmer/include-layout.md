# Include layout

Which header to reach for, and how to write the include line.

Developer counterpart: [../developer/file-layout.md](../developer/file-layout.md)

## One type, one header

`sq::gui::ScrollPane` is in `squared/gui/scroll_pane.hpp`. The file name is
the type name in `snake_case`, in the directory of its module, which is also
its namespace. There are no exceptions, so you never have to search for where
something is declared.

## The root, and `src/`

Each module keeps the headers you will use at its root, and the rest one
level down in `src/`:

```
squared/gui/button.hpp             sq::gui::Button: you include this
squared/gui/src/button_style.hpp   sq::gui::ButtonStyle: button.hpp already does
```

Everything in `src/` is still yours to include &mdash; it is out of the way,
not hidden. Every type there is reached through some root header, so in
practice you include the root one and get the rest. Write both the same way:

```cpp
#include <squared/gui/button.hpp>
#include <squared/gui/src/button_style.hpp>   // only if you name ButtonStyle on its own
```

The module pages list every type with its header, root or `src/`. Anything
under `src/detail/` is private to the framework and not listed.

## One include root

Every include line starts with `squared/`, so the compiler needs one `-I`: the
directory that contains `squared/`. A generated project's build already passes
it (`mk/pkg_squared_core.mk`). If you build by hand:

```sh
g++ -std=c++20 -I path/to/dir-containing-squared main.cpp ...
```

Keep the directory named `squared`.

## Modules that merged

The 2D graphics types and the scene graph were their own modules until
Sprint 1. Code written before then needs two renames:

| Was | Now |
|---|---|
| `<squared/graphics2d/texture.hpp>`, `sq::graphics2d::Texture` | `<squared/graphics/texture.hpp>`, `sq::graphics::Texture` |
| `<squared/scene2d/actor.hpp>`, `sq::scene2d::Actor` | `<squared/gui/actor.hpp>`, `sq::gui::Actor` |

Only identifiers and paths change. A string that happens to contain one of
those names &mdash; a libGDX class key in a skin file, say &mdash; does not.

## Aggregate headers

Some headers pull in more than one type. All of them are pure `#include`
lists:

| Header | Contains |
|---|---|
| `squared/<module>/<module>.hpp` | everything in the module (`assets`, `files`, `gles`, `graphics`, `gui`, `math`, `messaging`, `time`) |
| `squared/gui/input.hpp` | the input-routing types: events, listeners, keys, modifiers |
| `squared/app/application.hpp` | `Application`, with the event and text-input types it uses |
| `squared/data/json.hpp` | parsing and writing, with the value and result types |

## Which one to use

In a **source file**, use whatever is convenient. Aggregate headers cost
compile time, not runtime.

In a **header of your own**, include the narrow ones. A header that pulls in
`gui.hpp` to name a `Widget*` forces every consumer to parse the whole widget
set. Android is the constraining target and peak compiler memory is a real
constraint on device, so this matters more here than it would on a desktop.

Better still, forward declare. Every squared header does this where it can:

```cpp
namespace sq::gui {
class Painter;
class Skin;
}
```

## Self-containment

Every header compiles on its own, with no prerequisite include. `make headers`
(or `tools/check_headers.sh`) proves it by compiling each one in isolation.
It also checks that every header under `src/` is reached through one at a
module root, which is what lets you include the root header and get the rest.
Run it before you commit a new header.
