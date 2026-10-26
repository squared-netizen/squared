# Standard library deviations

The project's rule: the standard library is the default, but priority 1
outranks it, and several facilities are disqualified on memory grounds in
framework internals.

This page is the audit of where the tree currently **keeps** a disqualified
facility, and why its footprint was or was not acceptable. It is a record of
the existing state, not of choices made during the file split.

## Kept, and why it is currently acceptable

| Facility | Site | Assessment |
|---|---|---|
| `std::unordered_map` | `Skin`, ten members | **Not acceptable.** Largest resident cost in the framework. See [priority-audit.md](priority-audit.md) item 4. |
| `std::unordered_map` | `Ui::captures_` (pointer id &rarr; widget) | Bounded by simultaneous contacts, typically &le; 10. A flat vector of pairs would be smaller and faster at this size. |
| `std::function` | 8 public members | Interaction paths, not per-frame. `Widget::TooltipFactory` is the exception worth fixing: every widget pays 32 bytes. |
| `std::string` | style names on 7 widget types | 32 bytes per widget for a short constant. Interning is the fix. |
| `std::string` | `FontResource::descriptor_path_` | One per font resource, not per widget. Acceptable. |
| `std::shared_ptr` | `DrawablePtr`, `FontPtr` | Genuine shared ownership across skins and widgets, but a handle into a skin-owned pool would remove the control block and the atomics. |
| `std::deque<Cell>` | `Table::cells_` | Chosen for reference stability: `Table::add` returns `Cell&` and later adds must not invalidate it. A chunked vector or a stable pool would give the same guarantee with better locality. |
| `std::map`, `std::string`, `std::vector`, `<filesystem>` | `gui/src/layout_file.cpp`, `tools/sqgen/` (7.0b) | Build-time only: they read layout files in `sqgen` and in tools. An app links none of it (nothing in it refers to `layout_file.cpp`'s functions, so the archive member stays out). |
| `std::unique_ptr` with a virtual destructor | `sq::App`'s model (`detail::ModelHolder`, 7.1) | Once a process: the App cannot name the model's type. A fixed buffer would need a size knob ([stage.md](stage.md)). |
| `std::string`, `std::vector`, `std::shared_ptr` (`gui::FontPtr`) | `Stage::Drawing` while the skin loads (7.1) | Once a window, in `attach`: paths, the font's pages, gui's font type. Nothing of it on a frame. |
| `std::unique_ptr`, `std::string`, `std::function` | `gui::instantiate` (7.0) | Once per widget when a layout is built, never per frame: today's widgets own children through `unique_ptr`, take `std::string` words, and a button holds its tap (a `TapCall`: hook, context, the line) in its `std::function`. A per-screen arena can replace them behind `instantiate` ([layouts.md](layouts.md)). |
| `std::vector` | throughout | Fine. This is the intended default. |
| `std::optional` | `drag_pointer_`, `content_insets` | Fine; no allocation. |
| `std::array` | `NinePatchDrawable::regions_` | Fine, and the right call: 9 regions inline, no allocation. |
| `std::span` | `FontResource::pages()` | Fine; non-owning view, exactly as intended. |

## Passed over

| Facility | Site | Why |
|---|---|---|
| `std::function` | `gui::LayoutHooks` (7.0) | Function pointers and one context: nothing allocated to hold a hook, no type erasure. |
| `std::function` | `app::Report` (7.1) | The same: a function pointer and a context, 16 bytes. |
| `std::vector` | `sq::Stage`'s stack and waiting navigations (7.1) | Fixed at 16 screens and 4 changes: `std::array`, no growth, no allocation. |
| `<filesystem>`, `<fstream>` | `files::PosixFileSystem` (2026-10-19) | Measured on the phone: +613 KB in the demo's stripped `libdemo.so` (+55%), 79 allocations per iteration of everyday file work against 9, for a third fewer statements; and libc++ and libstdc++ answered one `create_directories` case differently. POSIX calls kept. Tools and tests may use both. The numbers: [files.md](files.md#posixfilesystem) |

`sq::Result`, an interned string table, a flat hash map, and a handle pool are
all named in the project rules and none exist yet. That is the gap between the
rules and the code, and it is tracked in the priority audit.

## `NinePatchDrawable` is worth a note

`sizeof(NinePatchDrawable)` is 400 bytes, driven by `std::array<TextureRegion,
9>` at 40 bytes per region. The nine regions are derived from one source
region plus four split values, so they are recomputable from 56 bytes. Storing
them precomputed trades 344 bytes per drawable for nine multiply-adds per
draw &mdash; a priority-3 win bought with priority-1 currency, which the
priority order says is the wrong direction. Skins typically hold tens of
nine-patches, so this is kilobytes, not megabytes; recorded, not urgent.
