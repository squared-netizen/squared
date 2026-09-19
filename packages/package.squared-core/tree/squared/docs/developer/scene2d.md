# scene2d &mdash; internals

Actor/Group/Stage composition and the input event contract.

Programmer counterpart: [../programmer/scene2d.md](../programmer/scene2d.md)

- Public types: 11
- Translation units: 4

## Layout

```
scene2d/
  include/squared/scene2d/   one header per public type, plus scene2d.hpp
  src/                        one translation unit per type with definitions
  Makefile
```

Types with no out-of-line definitions have no `.cpp`. Adding one means adding
its file to `SOURCES` in the module's `Makefile`.

## Downcasting without RTTI

`Actor::actor_interface(ActorInterfaceId)` is the framework's replacement for
`dynamic_cast`. Each Actor-derived interface publishes a
`static constexpr ActorInterfaceId actor_interface_id` and overrides the query
to answer its own identifier, delegating to its direct base otherwise:

```cpp
[[nodiscard]] void* actor_interface(ActorInterfaceId id) noexcept override
{
    return id == actor_interface_id ? this : Actor::actor_interface(id);
}
```

Delegating up the chain is what keeps an interface published anywhere in the
chain reachable: `actor_cast<Group>` on a `Widget` still resolves, because
`Widget` hands the unknown identifier to `Group`.

`scene2d::actor_cast<Interface>(actor)` supplies the identifier and restores
the pointer type. It accepts null and has a `const` overload.

Cost: one virtual call, no storage. `Actor` already had a vtable, so the two
extra slots are free, and the call is cheaper than the `dynamic_cast` it
replaced. Identifiers are four-character constants owned by the layer that
publishes the interface; the framework reserves values whose high byte is
`0x00`. `Group` uses `0x47'52'50'01`, `gui::Widget` uses `0x57'44'47'54`.

**Adding a layer:** derive from `Actor` or `Group`, publish an identifier,
override both overloads, and delegate to your direct base. Nothing in Scene2D
needs to know your type exists.

## Notes

`Actor` and `Group` are mutually recursive: `Actor` forward
declares `Group` for its parent pointer, `Group` includes `Actor` because it
derives from it. `InputEvent` forward declares `Actor` for the same reason,
which is what lets `input_listener.hpp` stay a two-line header.
