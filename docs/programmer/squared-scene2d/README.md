# Squared Scene2D — Programmer Guide

Squared Scene2D provides an owned two-dimensional actor hierarchy with
ordered traversal, hit testing, and portable capture-target-bubble input
dispatch. The package is self-contained: it has no GUI dependency and no
platform backend; rendering, transforms, focus ownership, actions, layouts,
skins, and widgets remain separate layers. Coordinates are translation-only
in this slice.

## Package availability

| Module | Version | Requires |
| --- | --- | --- |
| `dev.squarednetizen.squared.scene2d` | `0.6.0-dev.8` | `dev.squarednetizen.squared.graphics2d` `0.6.0-dev.8` |

The CMake target is `squared_scene2d`, a `STATIC` library exporting the
`include/` directory and the C++20 requirement.

## Public API overview

| Type | Header | Purpose |
| --- | --- | --- |
| `squared::scene2d::Actor` | `squared/scene2d/actor.hpp` | Base node with parent-relative bounds, visibility, touchability, frame traversal, and input listeners. |
| `squared::scene2d::Group` | `squared/scene2d/group.hpp` | Actor owning children in deterministic insertion order. |
| `squared::scene2d::Stage` | `squared/scene2d/stage.hpp` | Root owner for a hierarchy; logical viewport, traversal, hit testing, and input dispatch. |
| `squared::scene2d::InputType` | `squared/scene2d/input.hpp` | Event kind: pointer move/down/up/cancel, key down/up, navigation. |
| `squared::scene2d::InputKey` | `squared/scene2d/input.hpp` | Portable hardware-independent key names. |
| `squared::scene2d::NavigationAction` | `squared/scene2d/input.hpp` | Semantic focus and navigation action from any supported device. |
| `squared::scene2d::InputPhase` | `squared/scene2d/input.hpp` | Dispatch phase: capture, target, bubble. |
| `squared::scene2d::InputModifiers` | `squared/scene2d/input.hpp` | Backend-neutral modifier state. |
| `squared::scene2d::InputEvent` | `squared/scene2d/input.hpp` | Mutable event routed through a Stage actor path. |
| `squared::scene2d::InputListener` | `squared/scene2d/input.hpp` | `std::function` callback invoked per phase. |
| `squared::scene2d::InputListenerId` | `squared/scene2d/input.hpp` | Stable actor-local listener identifier. |

## Building an actor tree

A `Stage` owns a root `Group`. `Group` owns its children through
`std::unique_ptr`; `add_actor` takes ownership and returns a reference to the
adopted actor. An actor can never have more than one parent.

```cpp
#include <squared/scene2d/stage.hpp>

#include <memory>

squared::scene2d::Stage stage(960.0F, 540.0F);

auto card = std::make_unique<squared::scene2d::Actor>();
card->set_bounds(40.0F, 30.0F, 80.0F, 60.0F);
squared::scene2d::Actor& owned_card =
    stage.add_actor(std::move(card));

auto deck = std::make_unique<squared::scene2d::Group>();
deck->set_bounds(0.0F, 0.0F, 960.0F, 540.0F);
squared::scene2d::Group* deck_pointer = deck.get();
stage.add_actor(std::move(deck));

auto sleeve = std::make_unique<squared::scene2d::Actor>();
sleeve->set_bounds(0.0F, 0.0F, 80.0F, 60.0F);
deck_pointer->add_actor(std::move(sleeve));
```

Removal returns ownership and clears the parent link; the caller may re-attach
the detached actor elsewhere or let the `std::unique_ptr` destroy it.

```cpp
std::unique_ptr<squared::scene2d::Actor> detached =
    stage.root().remove_actor(owned_card);
if (detached) {
    detached->set_position(200.0F, 200.0F);
    stage.root().add_actor(std::move(detached));
}
```

`remove_actor` is linear and matches an immediate child by raw identity, so a
non-child (or a descendant reached through another group) returns a null
`std::unique_ptr`. `child_count()`, `child_at(index)`, and `clear()` complete
the management surface.

## Acting and traversal

`Stage::act(delta_seconds)` advances the whole hierarchy once. Traversal is
pre-order: each `Actor::act` runs before its children, and children run in
insertion order. Override `act` in a subclass to define per-frame behavior.

```cpp
#include <squared/scene2d/stage.hpp>

class Bouncer final : public squared::scene2d::Actor {
public:
    void act(double delta_seconds) override
    {
        set_position(
            x() + 100.0F * static_cast<float>(delta_seconds),
            y()
        );
    }
};

squared::scene2d::Stage stage(320.0F, 180.0F);
stage.add_actor(std::make_unique<Bouncer>());
stage.act(0.016);
```

`delta_seconds` is elapsed domain time in seconds. Setting `visible(false)`
removes an actor from traversal participation (and hit tests); the default
`act` on `Actor` and `Group` performs no transform, so behavior comes from
subclass overrides.

## Coordinate spaces

An actor's `x()` and `y()` are expressed in its parent's coordinate system.
`Stage` coordinates are root coordinates: the root group is positioned at
`(0, 0)` with the stage's logical size.

- `Stage::hit(stage_x, stage_y)` takes stage coordinates.
- `Actor::hit(local_x, local_y)` takes coordinates local to that actor.
- During dispatch, `InputEvent::local_x()` and `local_y()` are the pointer
  position relative to `current_target()`, recomputed for every actor on the
  path by summing the actor's ancestry positions.

```cpp
squared::scene2d::Stage stage(320.0F, 180.0F);
auto marker = std::make_unique<squared::scene2d::Actor>();
marker->set_bounds(10.0F, 20.0F, 80.0F, 60.0F);
squared::scene2d::Actor* marker_pointer = marker.get();
stage.add_actor(std::move(marker));

// Stage-space query; the marker occupies stage x in [10, 90), y in [20, 80).
squared::scene2d::Actor* hit = stage.hit(25.0F, 30.0F);
// The same actor, queried in its own local space:
squared::scene2d::Actor* local = marker_pointer->hit(15.0F, 10.0F);
```

## Hit testing

`hit` returns the deepest eligible actor at a coordinate. Invisible actors are
never returned. The `require_touchable` flag (default `true`) also skips
actors whose `touchable` flag is `false`; pass `false` to ignore touchability.

`Group::hit` converts its local coordinate into each child's local coordinate
and searches children from last to first, so the last child added is
topmost. If no child qualifies but the group itself contains the point, the
group is returned.

```cpp
squared::scene2d::Stage stage(320.0F, 180.0F);
auto base = std::make_unique<squared::scene2d::Actor>();
base->set_bounds(0.0F, 0.0F, 100.0F, 100.0F);
squared::scene2d::Actor* base_pointer = base.get();
stage.add_actor(std::move(base));

auto overlay = std::make_unique<squared::scene2d::Actor>();
overlay->set_bounds(20.0F, 20.0F, 40.0F, 40.0F);
squared::scene2d::Actor* overlay_pointer = overlay.get();
stage.add_actor(std::move(overlay));

squared::scene2d::Actor* top = stage.hit(30.0F, 30.0F);  // overlay
overlay_pointer->set_touchable(false);
top = stage.hit(30.0F, 30.0F);                           // base
top = stage.hit(30.0F, 30.0F, false);                    // overlay
overlay_pointer->set_visible(false);
top = stage.hit(30.0F, 30.0F);                           // base
squared::scene2d::Actor* none = stage.hit(500.0F, 500.0F);  // nullptr
```

## Input listeners and focus

Any actor can own regular and capture listeners. `add_input_listener` returns
a stable actor-local `InputListenerId` (monotonic within that actor) for later
removal; `capture = true` registers the callback for the capture phase only.

```cpp
#include <squared/scene2d/stage.hpp>

squared::scene2d::Stage stage(320.0F, 180.0F);
auto button = std::make_unique<squared::scene2d::Actor>();
button->set_bounds(10.0F, 10.0F, 60.0F, 30.0F);
squared::scene2d::Actor* button_pointer = button.get();
stage.add_actor(std::move(button));

bool clicked = false;
const auto click_id = button_pointer->add_input_listener(
    [&clicked](squared::scene2d::InputEvent& event) {
        if (event.type == squared::scene2d::InputType::pointer_down) {
            clicked = true;
            event.handle();
        }
    }
);

// ... dispatch pointer events through stage.dispatch_input(event) ...

const bool removed = button_pointer->remove_input_listener(click_id);
```

Listeners are notified from a snapshot taken at dispatch time, so an actor may
add or remove its own listeners during notification without invalidating the
current dispatch. A null callback throws `std::invalid_argument`.

Scene2D defines the portable *semantic* input types used for focus movement
(`InputType::navigation`, `NavigationAction`) but does not itself track
keyboard focus: focus ownership lives in higher layers (currently the Squared
GUI package, which owns a `scene2d::Stage` and routes navigation into it).

## Stage input dispatch

`Stage::dispatch_input(event, target)` routes one event through three phases:

1. **Capture** — capture listeners run from the root toward the target's
   parent.
2. **Target** — the target's capture listeners, then the target's virtual
   `input_event` handler and its regular listeners.
3. **Bubble** — virtual handlers and regular listeners run from the parent
   back to the root.

For pointer events, a null `target` selects the dispatch target by stage hit
testing at `event.stage_x`/`event.stage_y`. Other event types require an
explicit target. The stage resets the event's phase, flags, target, and
actor-local coordinates before dispatch.

```cpp
#include <squared/scene2d/stage.hpp>

squared::scene2d::Stage stage(320.0F, 180.0F);
auto leaf = std::make_unique<squared::scene2d::Actor>();
leaf->set_bounds(5.0F, 7.0F, 40.0F, 30.0F);
squared::scene2d::Actor* leaf_pointer = leaf.get();
stage.add_actor(std::move(leaf));

squared::scene2d::InputEvent event;
event.type = squared::scene2d::InputType::pointer_down;
event.pointer_id = 3;
event.stage_x = 18.0F;   // inside the leaf, in stage coordinates
event.stage_y = 20.0F;

const bool handled = stage.dispatch_input(event);
// event.target() is the hit leaf; event.handled_by() is the actor that first
// handled it; within a callback, event.local_x()/local_y() are actor-local.
```

An `InputEvent` is mutable and reusable: `handle()` records the first handling
actor without stopping other listeners, `stop()` ends propagation, and
`cancel()` handles and stops while marking the event cancelled. After a
dispatch, `target()`, `current_target()`, `handled_by()`, `phase()`,
`handled()`, `stopped()`, and `cancelled()` describe the outcome.

The GUI package demonstrates the intended bridge: `Widget` subclasses
`scene2d::Group`, overrides the protected virtual `input_event`, and adapts
the scene2d event into widget pointer/key handlers, calling `handle()` and
`stop()` when a widget consumes the event.

## Removal during traversal

`act()` iterates children in insertion order and dispatch iterates a
fixed actor path. Removing children from inside an `act` override is not yet
supported (the framework owns the active iteration), and structural hierarchy
mutation on the active dispatch path remains a tracked hardening item:
listeners should not destroy actors on the propagation path yet. Detaching
children between frames — for example from a parent that is not currently
traversing — is fully supported.

## Errors and failure behavior

- `Group::add_actor` throws `std::invalid_argument` when the argument is null
  or already has a parent. If the internal grow of the children container
  throws, the child is left unparented and the exception is rethrown (strong
  guarantee).
- `Actor::add_input_listener` throws `std::invalid_argument` when the callback
  is empty.
- `Stage::dispatch_input` returns `false` when there is no target (for
  example a pointer event outside every actor, or a non-pointer event with no
  explicit target) or when the target is not reachable from the stage root.
  Otherwise it returns whether the event was handled.
- `remove_actor` returns a null `std::unique_ptr` for a non-child;
  `remove_input_listener` returns `false` for an unknown identifier; `hit`
  returns `nullptr` when nothing qualifies.
- Bounds setters clamp negative sizes to zero rather than failing.

## Ownership and lifetime

- The parent `Group` (or the `Stage` root) owns each actor through
  `std::unique_ptr`; destroying or clearing a group destroys its children.
  `Actor` is non-copyable and non-movable, so a node cannot escape its owner
  except through the ownership-transfer methods.
- `remove_actor` returns ownership to the caller. The actor's parent link is
  cleared, so the detached node is ready to be re-added.
- `Stage` owns its root `Group` by value for the stage's lifetime.
- `InputEvent` is caller-owned; the stage only inspects and mutates it during
  `dispatch_input`. Listeners are owned by the actor and destroyed with it.
- Widget consumers such as the GUI hold raw pointers to actors for as long as
  the owning hierarchy lives; use the returned references to keep those
  pointers valid.

## Threading

All types must be used from one thread — the main thread that advances the
game loop, dispatches input, and owns the scene. There is no internal
synchronization; listener callbacks run synchronously on the dispatching
thread.

## Lua bindings

None of the types in this package have a Lua 5.4 binding.

## Related documentation

- Package payload: [Scene2D.md](../../../packages/squared-scene2d/content/docs/Scene2D.md)
- Implementation details: [Squared Scene2D — Developer Guide](../developer/squared-scene2d/README.md)
- Documentation index: [Programmer documentation](../README.md)
