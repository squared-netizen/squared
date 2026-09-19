# scene2d

Actor/Group/Stage composition and the input event contract.

Every type below lives in its own header. Include exactly the ones a
translation unit names; `squared/scene2d/scene2d.hpp` pulls in all of them
and exists for convenience, not for use inside headers of your own.

Developer counterpart: [../developer/scene2d.md](../developer/scene2d.md)

| Type | Header | Purpose |
|---|---|---|
| `ActorInterfaceId` | `squared/scene2d/actor_interface_id.hpp` | Stable identifier for one layer's Actor-derived interface |
| `Actor` | `squared/scene2d/actor.hpp` | Base node with parent-relative bounds and frame traversal |
| `Group` | `squared/scene2d/group.hpp` | Actor that owns children in deterministic insertion order |
| `InputEvent` | `squared/scene2d/input_event.hpp` | Mutable event routed through a Stage actor path |
| `InputKey` | `squared/scene2d/input_key.hpp` | Portable hardware-independent key names for key events |
| `InputListener` | `squared/scene2d/input_listener.hpp` | Callback invoked for each phase during input dispatch |
| `InputListenerId` | `squared/scene2d/input_listener_id.hpp` | Stable actor-local listener identifier; insertions increase it |
| `InputModifiers` | `squared/scene2d/input_modifiers.hpp` | Backend-neutral modifier state copied at the input boundary |
| `InputPhase` | `squared/scene2d/input_phase.hpp` | Dispatch phase of an event inside a Stage |
| `InputType` | `squared/scene2d/input_type.hpp` | Event kind carried by one InputEvent |
| `NavigationAction` | `squared/scene2d/navigation_action.hpp` | Semantic focus and navigation action from any supported device |
| `Stage` | `squared/scene2d/stage.hpp` | Root owner for a translation-only two-dimensional actor hierarchy |

## Downcasting an Actor

squared builds with `-fno-rtti`, so `dynamic_cast` is not available. Use
`actor_cast` instead:

```cpp
for (std::size_t i = 0; i < group.child_count(); ++i) {
    if (auto* widget = sq::scene2d::actor_cast<sq::gui::Widget>(
            group.child_at(i))) {
        // this child is a Widget
    }
}
```

It accepts null, returns null when the actor does not publish the requested
interface, and has a `const` overload. `Group` and `sq::gui::Widget` both
work as the target type.

To make your own `Actor` subclass reachable the same way, publish an
identifier and override both `actor_interface` overloads, delegating to your
direct base for identifiers you do not recognise:

```cpp
class MyNode : public sq::scene2d::Group {
public:
    static constexpr sq::scene2d::ActorInterfaceId
        actor_interface_id = 0x4D'59'4E'44U;

    [[nodiscard]] void* actor_interface(
        sq::scene2d::ActorInterfaceId id) noexcept override
    {
        return id == actor_interface_id ? this : Group::actor_interface(id);
    }

    [[nodiscard]] const void* actor_interface(
        sq::scene2d::ActorInterfaceId id) const noexcept override
    {
        return id == actor_interface_id ? this : Group::actor_interface(id);
    }
};
```
