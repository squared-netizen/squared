# Design patterns in use

Each entry names the pattern, what it was chosen over, how it deviates from
the textbook form, and what it costs.

## Composite &mdash; `Actor` / `Group` / `Widget`

Children are owned by `Group` through `std::vector<std::unique_ptr<Actor>>`;
`Widget` extends the composite with layout, paint and input.

- **Over:** a flat entity list with parent indices, which would have been
  smaller but would not give the libGDX feel the project asks for.
- **Cost:** 8 bytes of vtable pointer plus 24 bytes of vector per group node;
  `sizeof(Group)` is 96 bytes against `sizeof(Actor)` at 72.

## Template Method &mdash; `Widget::layout` / `paint` / `*_size`

The base defines the sequence, subclasses override the steps.

- **Over:** a strategy object per widget, which would have added an indirection
  and an allocation.
- **Cost:** one virtual call per widget per phase per frame. See priority audit
  item 7.

## Strategy (bounded) &mdash; `Drawable`

`ColorDrawable`, `RegionDrawable`, `NinePatchDrawable` behind one interface.

- **Over:** a tagged union. Three known kinds would fit a union in 400 bytes
  with no vtable and no `shared_ptr`. The project rules prefer the union here.
- **Cost:** vtable pointer plus a 16-byte `shared_ptr` at each reference site,
  and an atomic refcount per copy.

## Bridge &mdash; `Painter`

The GUI names no rendering backend; `Painter` is the entire boundary.

- **Deviation:** none worth noting. This is the pattern working as intended,
  and it is why the widget set has no SDL, GL or GLES dependency anywhere.
- **Cost:** one virtual call per draw primitive.

## Flyweight &mdash; `Skin` drawables and fonts

Shared immutable resources referenced by many widgets.

- **Deviation:** sharing is by `shared_ptr` rather than by handle into a
  skin-owned pool, so each reference costs 16 bytes and an atomic instead of 4
  or 8 bytes and nothing.

## Mediator &mdash; `ButtonGroup`

Coordinates checked state across toggle buttons without owning them; group and
buttons detach from each other on destruction, so neither imposes a second
ownership hierarchy.

- **Deviation:** the textbook mediator owns or outlives its colleagues. This
  one deliberately does not, which is the right call &mdash; it lets a group
  live in an ordinary widget subtree.
- **Cost:** 40 bytes plus one pointer per member button.

## Observer &mdash; `gui::InputListener`, widget callbacks

Low-frequency events only: input, lifecycle, value changes. Not used for
per-frame data flow, which matches the project rule.

- **Deviation:** the rule says "never via `std::function` lists", and
  `Actor::add_input_listener` stores exactly that.

## Null Object &mdash; style fallback

Every style lookup falls back to the style named `default`, so widgets never
branch on a missing style.

- **Cost:** the fallback is `map::at("default")`, which throws when `default`
  is itself absent. A genuine null object would return a static default-
  constructed style instead and could not throw.

The policy behind these choices, and the test for when an extension point is
warranted at all, is in [extension-policy.md](extension-policy.md).

## Builder &mdash; `LayoutWriter` / `NodeWriter`

C++ screens write a layout through a writer: `add<W>()` returns a
`NodeWriter` whose calls chain (`.bind(...).grow()`). Deviates from the
textbook in what it builds: not an object graph but a flat node table in
storage the caller gives, the same table a compiled layout file is. Cost: 192
bytes for the writer, 16 for a `NodeWriter` (a pointer and an index); nothing
allocated. Chosen over building widgets directly from C++, which would have
been a second way to make a screen ([layouts.md](layouts.md)).

## Attorney &mdash; `detail::FileWriting`

A layout file needs two writer calls the typed API rightly refuses at compile
time: adding a node by its `NodeType`, and binding by a name rather than a
`Key`. `LayoutWriter` and `NodeWriter` befriend one struct,
`gui::detail::FileWriting`, defined only in `layout_file.cpp`, which forwards
exactly those two calls to private members. The textbook attorney grants a
named client a subset of a class's privates; here the client is a whole
translation unit, and nothing outside it can call the attorney. Chosen over
making the calls public (an app could then bypass the checks the typed API
makes) or a second writer for files (two writers would have to agree). Cost:
two `friend` lines; no bytes, no indirection.

## Facade &mdash; `sq::App`

An app sees three things: `stage`, `run<Model>()` and `quit()`. Behind them
the App holds the Telegraph line, the model's runner and the Stage, and routes
the platform's lifecycle to each (it is the platform's
`sq::app::Application`). Cost: the vtable of that interface, one per process,
and nothing per call beyond the virtual dispatch the platform already made.
Chosen over handing the platform the Stage and the model separately, which
would make every platform layer know the start and shutdown order
([stage.md](stage.md)).

## Command queue (bounded) &mdash; the Stage's waiting navigations

A tap runs inside the button it tapped; building the next screen there would
free the button mid-call. Navigation asked for during an event goes into a
fixed queue of 4 (a `TapKind` and a layout pointer each, 64 bytes), applied
after delivery. Deviation from the textbook: no command objects, no undo, no
allocation; a full queue drops and reports. The demo's `ScreenStack` used a
`std::vector` of owning `unique_ptr<Screen>`s for the same job.

## Type erasure &mdash; `detail::ModelHolder`

The App holds a `ModelRunner<M>` without knowing `M`: `ModelOf<M>` derives
from `ModelHolder` (a virtual destructor, a virtual `turn()`), made once by
`run`. Cost: one allocation for the process's model, a vtable, and one
virtual call a frame for a model on the UI thread. Chosen over a fixed buffer
in the App (a size knob users would hit) and over `App<Model>`.

## Factory over a type tag &mdash; `instantiate`

Widgets are made from nodes by a `switch` on `NodeType`, with custom types
through one hook (a function pointer). Chosen over a registry of factories or
an abstract factory: the built-in types are fixed, so a registry would be an
extension point with no second use (priority 4). Cost: one branch per node,
at push time.

## Not used, deliberately

**Service Locator** &mdash; absent, as the rules require. `Ui` receives its
`TextInputService` explicitly and every subsystem takes its dependencies as
parameters. Worth protecting.

**ECS** &mdash; absent from the GUI, correctly. A widget tree is touched as
whole units, not iterated per component.

**Abstract Factory** &mdash; absent. `Widget::TooltipFactory` is a plain
callback, not a factory hierarchy.
