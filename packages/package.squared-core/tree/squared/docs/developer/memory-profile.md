# Memory profile

Measured on the host with `g++ -O2 -std=c++20`, 64-bit. An Android 32-bit ABI
will differ: pointers halve, so the pointer-heavy types shrink and the
float-only value types do not.

Regenerate with `tools/measure_sizes.sh`.

| Type | sizeof | alignof |
|---|---:|---:|
| `gui::Size` | 8 | 4 |
| `gui::Rectangle` | 16 | 4 |
| `gui::Insets` | 16 | 4 |
| `gui::SizeHints` | 24 | 4 |
| `graphics::Color` | 16 | 4 |
| `gui::PointerEvent` | 32 | 8 |
| `gui::TooltipConfig` | 40 | 8 |
| `gui::FontResource` | 312 | 8 |
| `gui::ColorDrawable` | 48 | 8 |
| `gui::RegionDrawable` | 32 | 8 |
| `gui::NinePatchDrawable` | 400 | 8 |
| `gui::DrawablePtr` | 16 | 8 |
| `gui::FontPtr` | 16 | 8 |
| `gui::ButtonStyle` | 128 | 8 |
| `gui::LabelStyle` | 56 | 8 |
| `gui::WindowStyle` | 176 | 8 |
| `gui::Skin` | 688 | 8 |
| `gui::Actor` | 72 | 8 |
| `gui::Group` | 96 | 8 |
| `gui::Widget` | 136 | 8 |
| `gui::Label` | 208 | 8 |
| `gui::Button` | 304 | 8 |
| `gui::ToggleButton` | 344 | 8 |
| `gui::CheckBox` | 376 | 8 |
| `gui::TextField` | 264 | 8 |
| `gui::Slider` | 240 | 8 |
| `gui::ProgressBar` | 184 | 8 |
| `gui::Panel` | 168 | 8 |
| `gui::Cell` | 72 | 8 |
| `gui::Table` | 240 | 8 |
| `gui::LinearLayout` | 176 | 8 |
| `gui::ScrollPane` | 168 | 8 |
| `gui::List` | 384 | 8 |
| `gui::PaneFrame` (internal) | 168 | 8 |
| `gui::Ui` | 1048 | 8 |
| `gui::ButtonGroup` | 40 | 8 |
| `gui::InputEvent` | 80 | 8 |
| `gui::SkinLoadIssue` | 72 | 8 |
| `gui::LayoutNode` | 32 | 8 |
| `gui::Layout` | 72 | 8 |
| `gui::LayoutWriter` | 192 | 8 |
| `gui::LayoutHooks` | 32 | 8 |
| `graphics::TextureRegion` | 40 | 8 |
| `graphics::BitmapGlyph` | 48 | 8 |
| `graphics::GlyphPlacement` | 56 | 8 |
| `graphics::BitmapFontError` | 32 | 8 |
| `app::Event` | 88 | 8 |
| `files::FileHandle` | 48 | 8 |
| `files::PosixFileSystem` | 104 | 8 |
| `files::FileError` | 72 | 8 |
| `data::JsonValue` | 56 | 8 |
| `data::JsonError` | 64 | 8 |
| `assets::AssetError` | 72 | 8 |
| `messaging::Name` | 8 | 8 |
| `messaging::Address` | 16 | 8 |
| `messaging::NameTable` | 24 | 8 |
| `messaging::Telegram` | 32 | 8 |
| `messaging::TelegramRing` | 192 | 64 |
| `messaging::ValueCache` | 88 | 8 |
| `messaging::ModelList` | 96 | 8 |
| `messaging::ListWindow` | 24 | 4 |
| `messaging::TelegraphLine` | 88000 | 64 |
| `messaging::Wake` | 4 | 4 |
| `messaging::WaitStats` | 8 | 4 |
| `time::Timepiece` | 24 | 8 |
| `time::DeadlineQueue<int, 32>` | 784 | 8 |
| `console::Scrollback` | 56 | 8 |
| `console::InputLine` | 2648 | 8 |
| `console::Commands` | 2056 | 8 |
| `console::Groups` | 2192 | 8 |
| `console::Console` | 74592 | 8 |
| `console::TelegraphTools` | 15568 | 8 |
| `console::Bridge` | 17376 | 8 |
| `console::Quad` | 20 | 4 |
| `console::Frame` | 24 | 4 |
| `console::Renderer` | 16416 | 4 |
| `std::string` | 32 | 8 |
| `std::function<void()>` | 32 | 8 |

## Reading this

**Value types are right-sized.** `Size` at 8 bytes, `Rectangle` and `Insets`
at 16, `Color` at 16, `PointerEvent` at 32. These are packed sensibly and
there is nothing to reclaim.

**Widget instances are dominated by strings and callbacks.** `Widget` is 136
bytes: 96 from `gui::Group`, two bools, and a 32-byte
`std::function` tooltip factory that every widget pays for whether or not it
has a tooltip. `Button` at 304 bytes is `Widget` plus a 32-byte label string,
a 32-byte style-name string, a 32-byte callback, a 32-byte glyph string, and
two 16-byte `shared_ptr`s. `CheckBox` reaches 376 bytes.

A screen with 200 widgets is therefore roughly 50&ndash;70&nbsp;KB of widget
objects before any content. That is not alarming, but the composition of it
is: a large fraction is short strings and empty `std::function`s.

**`Skin` is 688 bytes of object plus heap.** Ten hash maps, each allocating a
bucket array on first insert and one node per entry, each node carrying a
32-byte `std::string` key. This is the first thing priority 1 points at.

**`Ui` is 1,048 bytes**, one instance per application. Not a concern.

**`NinePatchDrawable` is 400 bytes**, nine precomputed `TextureRegion`s that
are derivable from 56 bytes of source data.

## Enum underlying types

The 41 enums clang-tidy found wider than their values need are
`: std::uint8_t` since Sprint 2, task 2.3, and `performance-enum-size` keeps
new ones that way. Measured at the change: `gui::Cell` went from 80 to 72
bytes and `app::Event` from 96 to 88. Every other measured type kept its
size: the bytes the enums gave up became padding next to wider members.
Getting them back means ordering members by size, type by type, with the
table above as the check. `gui::PointerEvent` (32) is the first candidate,
because it holds several (`gui::Window`, the other, went in 7.7). `gui::InputEvent` went from 88
to 80 bytes when its four-byte modifiers became the one-byte `app::KeyModifiers`
(Sprint 3.1b).

## What allocates, when

| Site | When | From |
|---|---|---|
| `Group::add_actor` | every widget added | global `new`, via `unique_ptr` |
| `Skin::add_*` | skin load | global `new`, per map node |
| `Table::add` | every cell added | global `new`, `std::deque` chunk |
| `Ui::show_pane` | per pane shown: its entry in the `Ui`'s list (16 bytes) | global `new`, the vector's growth |
| compiled layouts (`sqgen`) | never: read-only data, 32 bytes a node plus text and numbers ([layouts.md](layouts.md#memory)) | none |
| `sq::App` | never: static storage in the platform layer, the Telegraph line inside it (92,224 bytes at the default knobs, `static_assert`ed against the line as at most line + 4,224; the Stage's 4,056 hold 2,960 bytes of what screens and panes keep) ([stage.md](stage.md#memory-profile)) | none |
| `App::create`, first start | the platform's path of the model's save | global `new`, one `std::string` |
| `App::run` | once, at the model's start: its runner, the model inside; for a model with `save`, its last save, freed at its `start` | global `new`, one `unique_ptr`, one `std::vector` |
| `App::background` | a model's save, from 8 KB on its thread's stack; the write's paths | the stack; global `new` for the paths |
| `App::save_state` (Android's save instance state) | the stack, written into the platform's 8 KB on the glue thread's stack; the template copies what was written with `malloc` for the glue, which hands it to Android and frees it | the stack; `malloc` in the template |
| `Stage::attach` | each window: the skin's textures and tables, and one `Drawing` (23,864 bytes with libc++, 24,048 with libstdc++; the C++ screens' 17,480-byte scratch, the 4,096-byte table of 128 bound values and 4 list slots of 160 among them); while it loads, each skin file copied once out of the compiled-in bytes (46 KB in all, freed when loaded) | global `new` |
| the default skin (`stage/src/default_skin.cpp`) | never: 46,305 bytes of read-only data in the program, mapped from its file ([stage.md](stage.md#the-default-skin-74)) | none |
| `EmbeddedFileSystem::read`, `list` | a copy of the file's bytes; the names listed | global `new`, the result's vector and strings |
| `Stage::open` | each pane opened: its `gui::PaneFrame` (168 bytes) through `Ui::show_pane`, and its widgets through `instantiate`; freed when it closes or its screen is left | global `new` |
| `gui::instantiate` | a layout built (a screen pushed, a pane opened): each widget, its words, each tap's line | global `new`; `LayoutWriter` and `check` allocate nothing ([layouts.md](layouts.md)) |
| `gui::List` | when made: a copy of its row (nodes, the layout's words and numbers); as it is first laid out, or made taller: a row table and its widgets for each row on screen and one more; never as it scrolls | global `new` |
| `Table::measure`, `layout` | the first layout at each depth of nesting, and one with more rows or columns than before at that depth; after that, never (until low memory frees them: `Table::release_kept`) | global `new`, a kept `Lines` per depth ([gui.md](gui.md#a-tables-layout-allocates-nothing-75b)) |
| the Stage's lists | each list shown: 4 bytes a row it asks for, sized when the screen or pane is built (7.8; was at its first frame with a height) | global `new`, one `std::vector` |
| `BatchPainter` | made with each window's `Drawing`: its scratch text layout, room for 64 glyphs on 4 lines (3,744 bytes, 7.8); grows only for a longer text | global `new`, two `std::vector`s |
| `Widget::set_tooltip` | per tooltip shown | global `new`, a fresh subtree each time |
| `std::function` assignment | per callback with captures past SBO | global `new` |
| `std::string` assignment | per name past SSO (15 bytes) | global `new` |

**Everything allocates from the global allocator.** There is no arena, no
pool, and no frame allocator anywhere in this tree. The project rules require
that every subsystem allocate from an arena it owns, sized at init, and that
the steady-state footprint be knowable before `main` runs. Neither holds
today.

Nothing allocates inside `Ui::paint` once text storage has grown. Until Sprint
3 it did: every text a widget measured or drew made a fresh `GlyphLayout`, two
allocations each, 8 to 10 per frame in the demo. The frame-allocation counter
found it; `gui.md` (BatchPainter) and `graphics.md` (GlyphLayout) have the fix.
`Ui::layout` and `Ui::update` can still allocate:
`Ui::focusable_widgets` returns a fresh `std::vector<Widget*>` on every focus
move, and tooltip materialisation builds a whole widget subtree. Both are
input-driven rather than per-frame, but both are on a path a user can trigger
every frame by holding a key.

## `static_assert` coverage

There are no size assertions anywhere in the tree. The project rules call for
`static_assert` on the size of every core struct so a regression fails the
build; the table above gives the current values to assert against. No ceiling
has been set for the Android budget, so the assertions would pin current
values rather than enforce a target.
