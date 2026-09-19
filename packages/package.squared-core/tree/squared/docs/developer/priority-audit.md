# Priority audit

The project's priority order is RAM efficiency, then API ergonomics, then
speed, then extensibility. This page records where the tree as it stands
contradicts that order, or contradicts the language rules the project sets for
itself. **None of these were changed during the file split**: every one is a
behavioural or API change, and hiding those inside a mechanical refactor would
make both impossible to review.

Counts are measured against this tree, not estimated.

## 1. Exceptions are live &mdash; contradicts the no-exceptions rule

23 `throw` statements, plus `std::unordered_map::at` in the style lookup path
(`style_or_default` falls back to `styles.at("default")`, which throws
`std::out_of_range` when a skin has no `default` style).

Affected: `FontResource`, `Skin::add_font`, `ButtonGroup`, `NinePatchDrawable`,
`Ui::set_tooltip_config`, `Ui::update`, the skin loader.

The framework rule is a `Result`-style return for recoverable failure, an
assert for programmer error, and hard failure only at startup. Every throw site
here is one of the first two.

## 2. RTTI &mdash; resolved

**Closed.** All 17 `dynamic_cast` expressions are gone and the tree builds
clean with `-fno-rtti`:

```sh
make SQUARED_ABI=-fno-rtti
```

They were three distinct questions, answered three ways:

| Was | Sites | Now |
|---|---:|---|
| `Actor*` &rarr; `Widget*` | 11 | `scene2d::actor_cast<Widget>()` |
| `Actor*` &rarr; `Group*` | 3 | `scene2d::actor_cast<scene2d::Group>()` |
| `Widget*` &rarr; `TextField*` | 3 | `Widget::wants_text_input()` |

`actor_cast` is a one-virtual-call downcast built on
`Actor::actor_interface(ActorInterfaceId)`: each derived interface publishes a
constant and answers for it, delegating to its base otherwise. It costs no
storage &mdash; `Actor` already had a vtable &mdash; and is cheaper than
`dynamic_cast` was. See [scene2d.md](scene2d.md).

The third replacement is an improvement rather than a translation: only
`TextField` could raise the soft keyboard before, and a virtual lets any
custom text-entry widget opt in.

## 3. `-fno-exceptions` does not build

`-fno-rtti` is clean as of the RTTI pass. `-fno-exceptions` is not:
**21 remaining errors**, all exception-related.

What is left, by kind:

| Kind | Count | Work |
|---|---:|---|
| `throw std::invalid_argument` on a precondition | 20 | assert instead; one line each |
| `catch (...)` rollback against `bad_alloc` | 6 blocks | dead under `-fno-exceptions`; delete |
| `catch (std::bad_alloc)` converted to an error code | 2 blocks | pre-check limits and `reserve()` instead |
| `FontResource` constructors validating external data | 3 throws | static factories returning a status |
| `styles.at("default")` in `style_or_default` | 1 | return a static default-constructed style |

The 20 precondition throws are null `unique_ptr` arguments (9), reversed
min/max limits (2), empty resource names (2), out-of-range nine-patch splits,
an empty input listener, an already-parented actor, a non-finite tooltip
config, and a tooltip factory returning a parented widget. Every one is
programmer error, which the framework rules say is an assert.

`FontResource` is the only genuine API change on the list. It does not need a
new `Result` type: `BitmapFont::parse(..., BitmapFontError&)` already
establishes the idiom this codebase uses for recoverable failure.

**External blocker:** `gui/src/skin_loader.cpp` wraps the whole load in
`catch (const std::exception&)`, and inside it calls `data::parse_json` from a
module outside this archive. That translation unit cannot build with
`-fno-exceptions` until `sq::data` is exception-free.

## 3b. `Context::present()` discarded surface loss &mdash; resolved

**Closed.** It returned `void`; it now returns `[[nodiscard]] bool`. The only
signal some devices give that the rendering surface has gone was being thrown
away, and the symptom is an application frozen on its last frame after a phone
call. Found by comparing the framework's declaration against `kit.opengl`,
which had it right.

## 4. `Skin` is the largest resident cost

`sizeof(Skin)` is 688 bytes, and that is before any content: ten
`std::unordered_map<std::string, T>` members, each of which heap-allocates a
control block on first insert and one node per entry, with a 32-byte
`std::string` key per node.

A skin with 40 drawables and 8 styles per widget kind pays roughly 100
allocations and several kilobytes of pointer-chased nodes, resident for the
life of the application. Priority 1 says this is the first thing to fix.

Shape of the fix: intern names to a 32-bit handle at load time and index a
dense `std::vector` per style kind. Lookup becomes an array index; the string
table exists once instead of once per map.

## 5. `std::function` on interaction paths

Eight `std::function` members across the public API: `Button::Callback`,
`ToggleButton::ChangeCallback`, `Slider::ChangeCallback`,
`Dialog::ResultCallback`, `Widget::TooltipFactory`, `scene2d::InputListener`,
and both skin resolvers. 32 bytes each, plus a heap allocation for any capture
past the small-buffer size, plus an indirect call.

None of these are per-frame in the strict sense &mdash; they fire on input, not
on tick &mdash; but `Widget::TooltipFactory` sits in the 136-byte `Widget`
base, so **every widget in the tree pays 32 bytes for it** whether or not it
has a tooltip. That is the cheapest real win available: move it to a side
table keyed by widget, and `Widget` drops to 104 bytes.

## 6. Style names are `std::string` members

`std::string style_{"default"}` appears in `Label`, `Panel`, `Button`,
`TextField`, `Slider`, `ProgressBar`, and `Window`. 32 bytes per widget to
store a name that is almost always the same short literal, and it is compared
by value on every lookup. An interned handle is 4 bytes and compares as an
integer.

## 7. Virtual dispatch on the layout and paint path

`minimum_size`, `preferred_size`, `maximum_size`, `layout`, `paint`, and
`pointer_event` are all virtual and all called per frame per widget through
`Ui::layout` and `Ui::paint`. This is the ergonomic model libGDX users expect,
and the priority order does put ergonomics above speed &mdash; but it also says
ergonomics must not cost memory, and the vtable pointer is already paid for by
`scene2d::Actor`. Recorded here as a known cost, not as a defect.

## 8. Other standard-library choices

`std::deque<Cell>` in `Table`, `std::shared_ptr` behind `DrawablePtr` and
`FontPtr` (16 bytes plus an atomic control block per resource),
`std::unordered_map<std::int64_t, Widget*>` for pointer captures in `Ui`. See
[standard-library-deviations.md](standard-library-deviations.md).

## What the file split changed

Nothing on this list. That refactor moved text between files and adjusted
include sets. Every declaration and every definition was byte-identical to its
input, verified by a round-trip check in the tooling.

## What the RTTI pass changed

Item 2 only. Nine files: a new `ActorInterfaceId` header, the interface query
on `Actor` and `Group`, the same on `Widget` plus `wants_text_input()`, the
`TextField` override, and the 17 call sites in `ui.cpp`, `widget.cpp` and
`stack.cpp`. `ui.cpp` also drops its `text_field.hpp` include, which is no
longer named there.

Behaviour is unchanged. `actor_cast` was tested against the cases
`dynamic_cast` used to answer: a plain `Group` is a `Group` and not a
`Widget`; a `Widget` answers both identifiers and returns the same `Group`
subobject `dynamic_cast` produced; an `Actor` that is neither answers neither;
null in, null out; and the `const` overload agrees with the mutable one.
