# Include layout

## One type, one header

```
<module>/include/squared/<module>/<type>.hpp
```

`sq::gui::ScrollPane` is in `squared/gui/scroll_pane.hpp`. The file name
is the type name in `snake_case`. There are no exceptions to this rule, which
means you never have to search for where something is declared.

## Aggregate headers

Three kinds of header pull in more than one type. All of them are pure
`#include` lists:

| Header | Contains |
|---|---|
| `squared/gui/gui.hpp` | every GUI type, as before the split |
| `squared/gui/skin_loader.hpp` | the skin-loading types and the loader functions |
| `squared/<module>/<module>.hpp` | every public type in that module |

Existing code that includes `squared/gui/gui.hpp` keeps compiling unchanged.

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
(or `tools/check_headers.sh`) proves it by compiling each one in isolation;
run it before you commit a new header.
