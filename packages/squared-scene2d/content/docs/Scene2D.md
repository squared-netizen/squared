# Squared Scene2D

Squared Scene2D begins the Phase 6 scene hierarchy as an independent compiled
`.sq` module. Version `0.6.0-dev.1` deliberately provides only the stable base:

- `Actor` owns parent-relative bounds, visibility, touchability, and frame
  traversal;
- `Group` owns children in deterministic insertion order and searches hits in
  reverse order so the last child is topmost;
- `Stage` owns the root group, its logical size, traversal, and stage-space hit
  testing.

Coordinates are translation-only in this first slice. Rendering, transforms,
event propagation, focus, actions, layouts, skins, and widgets are later Phase
6 layers. Keeping those out of the initial contract avoids placeholder APIs
that would constrain the renderer and input model prematurely.

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
