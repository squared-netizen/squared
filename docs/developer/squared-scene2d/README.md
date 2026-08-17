# Squared Scene2D — Developer Guide

Squared Scene2D is the portable retained hierarchy layer: an owned,
translation-only actor tree with pre-order traversal, reverse hit testing, and
a deterministic capture-target-bubble input dispatch. It is intentionally
independent of both the GUI package and any platform backend so that
application scenes can be built, advanced, and hit-tested without a rendering
stack.

- Programmer counterpart: [Squared Scene2D — Programmer Guide](../programmer/squared-scene2d/README.md)
- Package payload: [Scene2D.md](../../packages/squared-scene2d/content/docs/Scene2D.md)
- Documentation index: [Developer documentation](../README.md)
- Diagrams:
  - [Actor/group/stage ownership](ownership.dot)
  - [Hit-test traversal](hit-test.dot)
  - [Input propagation and focus transitions](input-focus.dot)
  - [Framework package dependencies](../architecture/package-dependencies.dot)

## Dependency boundary

The manifest declares one exact `module.requires`: `squared.graphics2d`
`0.6.0-dev.7`. The CMake target `squared_scene2d` links `squared_graphics2d`
(`content/modules/squared-scene2d/CMakeLists.txt`).

The code-level boundary is currently asymmetric with the manifest: no Scene2D
header or source includes a Graphics2D header today. `actor.hpp`, `group.hpp`,
`stage.hpp`, and `input.hpp` include only the standard library. The Graphics2D
dependency is therefore an exact package-coordinate and link-time edge laid
down in preparation for rendering and transforms, not a set of types this
package consumes yet. There is no include, symbol, or link path to the GUI
package or to a platform backend.

## Architecture

| Component | Source | Responsibility |
| --- | --- | --- |
| `squared::scene2d::Actor` | `include/squared/scene2d/actor.hpp`, `src/actor.cpp` | Base node: parent-relative bounds, visibility, touchability, virtual `act`/`hit`/`input_event`, and the actor-local listener set. |
| `squared::scene2d::Group` | `include/squared/scene2d/group.hpp`, `src/group.cpp` | Owns children in insertion order; pre-order traversal; reverse hit testing; ownership-transfer add/remove. |
| `squared::scene2d::Stage` | `include/squared/scene2d/stage.hpp`, `src/stage.cpp` | Owns the root `Group`, the logical viewport, stage-space hit testing, and capture-target-bubble dispatch. |
| `squared::scene2d::InputEvent` and enums | `include/squared/scene2d/input.hpp`, `InputEvent::handle` in `src/actor.cpp` | Backend-neutral event payload, phase flags, and listener id alias. |

`Actor` holds its listeners inline as a `std::vector<ListenerEntry>`; there is
no separate listener registry. `Group` holds `std::vector<std::unique_ptr<Actor>>`.
`Stage` holds a single `Group root_` by value plus no other mutable state.

## Ownership and threading

- **Parent owns child.** `Group::children_` is a `std::vector<std::unique_ptr<Actor>>`.
  `add_actor` sets the child's parent before insertion and, on a throwing
  `push_back`, resets the parent link and rethrows, so a failed adoption never
  leaves a half-attached child (strong exception safety).
- **Ownership transfers through moves, never copies.** `Actor` deletes both
  copy and move operations, so a node can only move between owners via
  `std::unique_ptr` held by `add_actor`/`remove_actor`/`clear`.
- **Stage owns the root.** `Stage` contains `Group root_` by value; `add_actor`
  and `hit` forward to it. `resize` sets the root's bounds to `(0, 0, w, h)`
  with the usual non-negative clamping.
- **Listeners are actor-local state.** `next_listener_id_` is a per-actor
  monotonic `std::uint64_t` starting at 1; ids never collide within an actor
  and are never reused after removal.
- **Main-thread only.** Traversal, dispatch, and mutation are unsynchronized;
  the application must confine all use to the thread that runs the stage.

## Invariants and failure behavior

- **Parent/child consistency.** A child's `parent_` is non-null exactly while
  its `unique_ptr` sits in that parent's `children_`. `add_actor` rejects null
  actors and already-parented actors with `std::invalid_argument`.
  `remove_actor` matches by raw identity linearly, detaches (`parent_ = null`),
  and returns ownership. `clear()` detaches every immediate child before
  destroying them.
- **Traversal invariants.** `Group::act` runs the group's own `Actor::act`
  first, then children in insertion order (pre-order, parent first). The
  iteration holds `children_` unmodified for the duration; mutation during
  `act` is not supported.
- **Hit-test invariants.** Invisible actors are never returned. `Actor::hit`
  requires containment in `[0, width) × [0, height)` (half-open) and,
  optionally, `touchable`. `Group::hit` first requires the group to contain
  the local point; it then probes children last-to-first and finally falls
  back to the group itself (`Actor::hit`) when no child qualifies, so the
  deepest visible eligible node wins and a bare group can be a hit target.
- **Dispatch invariants.** `Stage::dispatch_input` rebuilds the event state
  each call: phase, target, current target, flags, and actor-local coordinates
  are reset before dispatch, so `InputEvent` values are reusable. Dispatch
  requires the target's ancestor chain to terminate at `&root_`; a target not
  reachable from the root (for example a detached actor) returns `false`.
  Listener callbacks are copied into a snapshot before notification, allowing
  listener add/remove during notification without iterator invalidation; the
  snapshot holds `std::function` copies, so a removed listener is still
  invoked once for the in-flight dispatch.
- **Structural mutation on the active path is not safe.** The snapshot
  protects the listener container only. Destroying actors on the current
  propagation path or mutating `children_` during `act`/dispatch is tracked
  hardening work (`TODO.md`), not a supported behavior.

## Data structures and complexity

- `children_` — `std::vector<std::unique_ptr<Actor>>`:
  - `add_actor`: amortized O(1) `push_back` plus constant-time parent wiring.
  - `remove_actor`: O(n) `std::find_if` over raw pointers, then O(1) erase of
    the moved-out entry.
  - `clear`: O(n) detach, then O(n) destruction.
  - `child_at`: O(1) with bounds check.
- `Actor::listeners_` — `std::vector<ListenerEntry>`; insertion amortized O(1),
  removal O(k) by id, notification O(k) snapshot plus O(k) calls.
- `Stage::dispatch_input` — builds a path of length O(depth); per node it
  snapshots O(listeners) callbacks. Overall O(depth × listeners-per-node).
- `Stage::act`/`Stage::hit` — O(n) over the whole tree; hit is O(n) per level
  in the worst case (reverse scan stops at the first hit).

The `std::vector` containers were chosen for iteration locality and amortized
insertion; the cost is linear removal and index-invalidation of references,
which is why the API is raw-pointer and id based rather than index based.

## Algorithms and execution order

**Pre-order `act`.** `Group::act` calls `Actor::act(delta_seconds)` (virtual,
no-op at the base) and then each child in insertion order. `Stage::act` is
`root_.act`.

**Reverse hit test.** `Group::hit(local_x, local_y)` verifies visibility and
containment, then iterates `children_` with a reverse iterator, converting the
parent-local point into each child's local frame by subtracting the child's
position (`local_x - child.x()`, `local_y - child.y()`) before recursing.
First qualifying result wins; a group with no qualifying child falls back to
itself.

**Capture → target → bubble dispatch.** `Stage::dispatch_input`:

1. Resolve the target: an explicit `Actor*` argument, or for pointer events a
   stage hit test at `(stage_x, stage_y)` with `require_touchable = true`.
   Other event types without a target return `false`.
2. Walk `target` to `root_`, collecting the path.
3. Reset event flags/targets and record `target_`.
4. Capture phase: `notify_input(event, true)` on each path node from the root
   toward the target's parent.
5. Target phase: the target's capture listeners, then — unless stopped — the
   target's virtual `input_event` and regular listeners
   (`notify_input(event, false)`).
6. Bubble phase: `notify_input(event, false)` from the target's parent back to
   the root; `stop()` breaks out.
7. Return `event.handled()`.

Actor-local coordinates are computed per callback by summing the actor's
ancestry positions and subtracting from the stage position, so `local_x()`/
`local_y()` are exact for every node on the path regardless of nesting depth.
`notify_input(capture_only)` runs the virtual handler and regular listeners in
the target/bubble path and capture listeners only in capture; the virtual
`input_event` is invoked before external listeners and after capture listeners
at the target. The GUI `Widget` overrides `input_event` to bridge into widget
pointer/key handlers, so the subclass hook participates in the same
deterministic order.

## Design patterns

- **Composite** — the actor tree is a classic scene-graph Composite: `Group`
  is both a leaf-capable `Actor` and a container, every node has the same
  `act`/`hit` interface, and traversal recurses uniformly. Why it fits:
  retained scenes need uniform recursive updates and spatial grouping without
  a parallel per-node system. Alternatives rejected: a flat id-referenced
  entity list (loses recursive grouping and per-node behavior), and a
  data-oriented ECS (premature for the retained hierarchy). Deviation: the
  textbook Composite couples container and leaf APIs (`Add`/`Remove` returning
  children); here `Group` deliberately exposes only ownership-transfer add/
  remove, and a `Group` can itself be the result of a hit test.
- **Observer** — input listeners are per-actor observers notified in a
  deterministic phase order. Why it fits: multiple independent consumers
  (application logic, widgets) can observe the same actor without the actor
  knowing them. Alternatives rejected: a single global event bus (loses
  per-actor scoping and the capture/bubble ordering), and direct virtual-only
  dispatch (no external registration). Deviations: observers are snapshotted
  so reentrant add/remove during notification is safe; ids are stable for
  removal; and the *capture* set is a distinct observer set driven by the
  dispatch phase. The protected virtual `input_event` acts as a subclass hook
  (a hook-method) inside the target/bubble notification, giving derived
  classes a deterministic insertion point before external listeners.

## Limitations and technical debt

- **No sorting or layering API.** Z-order is insertion order only;
  reordering a child requires remove-and-re-add, which renumbers it. A
  dedicated `set_child_z`/`swap` API is not implemented.
- **Homogeneous children vector.** All children are `Actor*`; GUI widgets
  `dynamic_cast` back to their concrete type (see the GUI `validate_layout`
  walk), which is linear and type-unsafe by design.
- **Translation-only semantics.** Position, size, visibility, and
  touchability are the only per-node state; transforms, rotation, scale,
  rendering, and actions are future compatible layers.
- **Mutation during active traversal/dispatch is unsafe.** `TODO.md` records
  hierarchy-mutation tests during updates and dispatch as the next hardening
  item; until then, destroying actors on an active propagation path is
  unsupported.
- **No focus primitives in Scene2D.** Focus ownership belongs to the GUI
  package (`Ui` tracks `focused_`, focus moves, and `focus_changed`
  notifications over its own `scene2d::Stage`). `TODO.md` records defining
  focus ownership primitives for non-GUI Scene2D applications as unfinished.
- **Coordinate math is float and unclamped in `set_position`**; only sizes are
  clamped, so callers must keep positions meaningful for their own hit logic.
