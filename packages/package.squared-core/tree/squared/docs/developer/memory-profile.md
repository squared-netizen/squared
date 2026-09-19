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
| `scene2d::Actor` | 72 | 8 |
| `scene2d::Group` | 96 | 8 |
| `gui::Widget` | 136 | 8 |
| `gui::Label` | 208 | 8 |
| `gui::Button` | 304 | 8 |
| `gui::ToggleButton` | 344 | 8 |
| `gui::CheckBox` | 376 | 8 |
| `gui::TextField` | 248 | 8 |
| `gui::Slider` | 240 | 8 |
| `gui::ProgressBar` | 184 | 8 |
| `gui::Panel` | 168 | 8 |
| `gui::Cell` | 80 | 8 |
| `gui::Table` | 240 | 8 |
| `gui::LinearLayout` | 176 | 8 |
| `gui::ScrollPane` | 168 | 8 |
| `gui::Window` | 304 | 8 |
| `gui::Dialog` | 360 | 8 |
| `gui::Ui` | 1032 | 8 |
| `gui::ButtonGroup` | 40 | 8 |
| `graphics2d::TextureRegion` | 40 | 8 |
| `graphics2d::BitmapGlyph` | 48 | 8 |
| `graphics2d::GlyphPlacement` | 56 | 8 |
| `std::string` | 32 | 8 |
| `std::function<void()>` | 32 | 8 |

## Reading this

**Value types are right-sized.** `Size` at 8 bytes, `Rectangle` and `Insets`
at 16, `Color` at 16, `PointerEvent` at 32. These are packed sensibly and
there is nothing to reclaim.

**Widget instances are dominated by strings and callbacks.** `Widget` is 136
bytes: 96 from `scene2d::Group`, two bools, and a 32-byte
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

**`Ui` is 1,032 bytes**, one instance per application. Not a concern.

**`NinePatchDrawable` is 400 bytes**, nine precomputed `TextureRegion`s that
are derivable from 56 bytes of source data.

## What allocates, when

| Site | When | From |
|---|---|---|
| `Group::add_actor` | every widget added | global `new`, via `unique_ptr` |
| `Skin::add_*` | skin load | global `new`, per map node |
| `Table::add` | every cell added | global `new`, `std::deque` chunk |
| `Ui::show_window` | per window shown | global `new` |
| `Widget::set_tooltip` | per tooltip shown | global `new`, a fresh subtree each time |
| `std::function` assignment | per callback with captures past SBO | global `new` |
| `std::string` assignment | per name past SSO (15 bytes) | global `new` |

**Everything allocates from the global allocator.** There is no arena, no
pool, and no frame allocator anywhere in this tree. The project rules require
that every subsystem allocate from an arena it owns, sized at init, and that
the steady-state footprint be knowable before `main` runs. Neither holds
today.

Nothing allocates inside `Ui::paint`. `Ui::layout` and `Ui::update` can:
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
