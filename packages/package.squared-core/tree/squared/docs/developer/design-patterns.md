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

## Observer &mdash; `scene2d::InputListener`, widget callbacks

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

## Not used, deliberately

**Service Locator** &mdash; absent, as the rules require. `Ui` receives its
`TextInputService` explicitly and every subsystem takes its dependencies as
parameters. Worth protecting.

**ECS** &mdash; absent from the GUI, correctly. A widget tree is touched as
whole units, not iterated per component.

**Abstract Factory** &mdash; absent. `Widget::TooltipFactory` is a plain
callback, not a factory hierarchy.
