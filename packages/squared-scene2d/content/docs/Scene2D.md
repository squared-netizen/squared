# Squared Scene2D

Squared Scene2D provides an independent compiled `.sq` hierarchy and portable
input-routing module:

- `Actor` owns parent-relative bounds, visibility, touchability, and frame
  traversal;
- `Group` owns children in deterministic insertion order and searches hits in
  reverse order so the last child is topmost;
- `Stage` owns the root group, its logical size, traversal, and stage-space hit
  testing;
- `InputEvent` carries backend-neutral pointer, key, modifier, and semantic
  navigation data;
- actor listeners observe deterministic capture, target, and bubble phases.

Coordinates remain translation-only. Rendering, transforms, focus ownership,
actions, layouts, skins, and widgets remain separate layers.

## Ownership

Groups receive children through `std::unique_ptr`. Removing a child returns
that ownership and clears its parent link. Clearing or destroying a group
destroys its remaining children. An actor cannot be copied or moved while it
is attached to a hierarchy.

## Hit testing

`Actor::hit()` accepts coordinates local to the actor. `Group::hit()` converts
its local coordinate into each child's local coordinate and searches children
from last to first. Invisible actors are never returned. The caller may choose
whether the touchable flag is enforced.

## Input propagation

`Stage::dispatch_input()` accepts an explicit actor target. For pointer events
it can also find the target through stage hit testing. Dispatch then follows:

1. capture listeners from the root toward the target's parent;
2. target capture listeners, the target virtual handler, and target listeners;
3. virtual handlers and regular listeners from the parent back to the root.

`InputEvent::handle()` records the first handling actor without stopping other
listeners. `stop()` ends propagation, while `cancel()` handles and stops the
event. Stage updates actor-local pointer coordinates before every callback.
Listener callbacks are snapshotted, so an actor may add or remove listeners
during notification without invalidating the current dispatch.

Structural hierarchy mutation during dispatch remains a separately tracked
hardening item; listeners should not destroy actors on the active propagation
path yet.
