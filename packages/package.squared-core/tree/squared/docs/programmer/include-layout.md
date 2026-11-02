# Imports and file layout

How your code reaches squared's types, and where each one is written.

Developer counterpart: [../developer/file-layout.md](../developer/file-layout.md)

## One import

squared is modules, in C++23. An app writes one line:

```cpp
import squared;
```

and has the App and its Stage, the widgets and layouts, the Telegraph with
its keys, the application model, and the standard library: C++23's `std`
module, the whole of it (`std::string`, `std::map`, `std::print`, ...).

A narrower import names one module, `sq.<module>`, its namespace's name:

```cpp
import sq.gui;      // sq::gui: the widgets, Skin, layouts, the scene graph
import sq.files;    // sq::files: FileHandle, the file systems
import std;         // the standard library, alone
```

| Module | Namespace | What |
|---|---|---|
| `sq.app` | `sq::app` | the application model: Application, Event, the keys, text input |
| `sq.assets` | `sq::assets` | the asset manager |
| `sq.console` | `sq::console` | the developer Console |
| `sq.data` | `sq::data` | JSON |
| `sq.dev` | `sq::dev` | the dev log, frame allocations, crash traces |
| `sq.files` | `sq::files` | files and file systems |
| `sq.gles` | `sq::gles` | the GLES wrappers (gles backend) |
| `sq.graphics` | `sq::graphics` | textures, sprites, fonts, the camera |
| `sq.gui` | `sq::gui` | widgets, Skin, layouts, the scene graph |
| `sq.math` | `sq::math` | vectors and matrices |
| `sq.messaging` | `sq::messaging` | the Telegraph, names, keys |
| `sq.stage` | `sq` | the App and its Stage |
| `sq.time` | `sq::time` | clocks, timers, deadline queues |
| `std` | `std` | the standard library (C++23; libc++'s) |
| `squared` | | all of the above an app needs: std, app, gui, messaging, stage |

A module also gives you what its types are made of: `import sq.gui;` brings
the `sq::graphics` types a widget's interface names, as an include of a
widget header did.

An import is not text: the module was compiled once, when squared was built,
and your file reads from it only what it uses. That is why it is quicker than
the headers were.

### What still needs an include

**C macros.** A module carries no macros. `assert`, `errno` and its codes,
`va_start`, `UINT32_MAX`, `SIGSEGV`, `offsetof`, `stdout`: include the C
header that defines them, above the imports:

```cpp
#include <assert.h>

import squared;
```

**Every include above every import.** libc++'s C headers (`<stdio.h>`,
`<stdlib.h>`, `<string.h>`, `<math.h>`, ...) bring in parts of the C++ library
too, and read after the standard library came in by import they clash with
it: "type alias template redefinition". So a file includes first (system
headers, then your own headers, which may import), and imports last.

Nothing else needs an include: the `std` module is the whole standard
library, `std::isdigit` and the other `<cctype>` functions included (on
Android, with `-D__BIONIC_CTYPE_INLINE=inline`, which the build passes).

### Your flags

The build compiles your code the way the framework was compiled where it
matters: C++23, libc++ and `-pthread`. A file compiled with another standard,
or without RTTI or exceptions when squared has them, cannot read squared's
modules ("configuration mismatch"): keep `CXXSTD` at `c++23`. A generated
project's build does all of this (`mk/pkg_squared_core.mk`), and builds the
framework first: the modules are read from `squared/build/release/modules/`,
`std` among them (made once per compiler and flags, and shared by your
projects: [building.md](building.md)).

## One type, one file

`sq::gui::ScrollPane` is written in `squared/gui/scroll_pane.cppm`. The file
name is the type name in `snake_case`, in the directory of its module, which
is also its namespace. There are no exceptions, so you never have to search
for where something is declared.

Each module keeps the types you will use at its root, and the rest one level
down in `src/`:

```
squared/gui/button.cppm             sq::gui::Button
squared/gui/src/button_style.cppm   sq::gui::ButtonStyle: comes with Button
```

Both come with the module's import. The module pages list every type with its
file. Anything under `src/detail/` is the framework's own.

## Modules that merged

The 2D graphics types and the scene graph were their own modules until
Sprint 1. Code written before then needs two renames:

| Was | Now |
|---|---|
| `<squared/graphics2d/texture.hpp>`, `sq::graphics2d::Texture` | `import sq.graphics;`, `sq::graphics::Texture` |
| `<squared/scene2d/actor.hpp>`, `sq::scene2d::Actor` | `import sq.gui;`, `sq::gui::Actor` |

Code written before 7b.4 writes includes: each `#include <squared/<module>/...>`
becomes `import sq.<module>;`, and `#include <squared/squared.hpp>` becomes
`import squared;`. Standard includes become `import std;` (or nothing: `import
squared;` brings it), and `import sq.std;` (until 2026-11-02) becomes `import
std;`.
