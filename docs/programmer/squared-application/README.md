# Squared Application — Programmer Guide

Squared Application is the header-only, platform-neutral application
lifecycle and event boundary of the Squared framework. Developer application
code implements `squared::application::Application`; a generated platform
adapter owns operating-system and window-setup work, translates native input
and lifecycle changes into `squared::application::Event` value objects, and
calls the lifecycle hooks in a documented order. No SDL, OpenGL, Android, or
GUI type appears in the public headers. The module has no implementation
library and no third-party dependency.

## Package availability

| Module | Version | Requires |
| --- | --- | --- |
| `dev.squarednetizen.squared.application` | `0.6.0-dev.5` | (none) |

The CMake target is `squared_application`, an `INTERFACE` target exporting
the `include/` directory and the C++20 requirement.

## Public API overview

| Type | Header | Purpose |
| --- | --- | --- |
| `squared::application::Application` | `squared/application/application.hpp` | Lifecycle interface implemented by developer applications. |
| `squared::application::Event` | `squared/application/event.hpp` | Compact platform-neutral event aggregate. |
| `squared::application::Event::Type` | `squared/application/event.hpp` | Event category for the portable boundary. |
| `squared::application::Event::Key` | `squared/application/event.hpp` | Portable hardware-independent key names. |
| `squared::application::Event::Navigation` | `squared/application/event.hpp` | Semantic focus and navigation actions. |
| `squared::application::KeyModifier` | `squared/application/event.hpp` | Backend-neutral modifier flags. |
| `squared::application::KeyModifiers` | `squared/application/event.hpp` | Value object holding zero or more modifiers. |
| `squared::application::TextInputPurpose` | `squared/application/text_input.hpp` | Desired soft-keyboard content mode. |
| `squared::application::TextInputArea` | `squared/application/text_input.hpp` | Soft-keyboard target rectangle in logical pixels. |
| `squared::application::TextInputRequest` | `squared/application/text_input.hpp` | Request payload sent when text input starts. |
| `squared::application::TextInputService` | `squared/application/text_input.hpp` | Application-to-platform soft-keyboard boundary. |

## Implementing the lifecycle

`Application` is a pure interface. `create()` initializes logical state once
and receives the active rendering context; `handle_event()` receives one
portable event per call; `update()` advances logic by an elapsed duration;
`render()` draws one frame. `pause()`, `resume()`, `resize()`,
`surface_created()`, `surface_destroyed()`, `set_text_input_service()`, and
`dispose()` are optional overrides, supplied for the lifecycle hooks the
application needs. `quit_requested()` is queried by the platform event loop.

```cpp
#include <squared/application/application.hpp>

namespace squared::graphics {
class Context;
}

class Game final : public squared::application::Application {
public:
    bool create(squared::graphics::Context& graphics) override
    {
        (void)graphics;
        return true;
    }
    void handle_event(const squared::application::Event& event) override
    {
        if (event.type == squared::application::Event::Type::QuitRequested) {
            quit_ = true;
        }
    }
    void update(std::chrono::nanoseconds delta) override { (void)delta; }
    void render(squared::graphics::Context& graphics) override
    {
        (void)graphics;
    }
    void dispose() override {}
    bool quit_requested() const noexcept override { return quit_; }

private:
    bool quit_{false};
};
```

Lifecycle calls arrive in a strict order. `create()` runs once. After a
usable context exists the platform calls `surface_created()`; before
rendering becomes unavailable it calls `pause()` and then
`surface_destroyed()`, which must not assume graphics commands remain legal.
On foreground recovery the platform restores the context, calls
`surface_created()`, resizes the application, then calls `resume()`. The
example adapter in `examples/gui-showcase/android/platform/sdl_main.cpp`
follows exactly this ordering. `dispose()` releases logical state once and
may run while no rendering surface is available.

## Events and payloads

`Event` is one aggregate carrying a `type` and the payload fields relevant to
that category. Every member is value-initialized on the unused categories.

| `Event::Type` | Payload members | Meaning |
| --- | --- | --- |
| `QuitRequested` | — | The platform asks the application to end. |
| `PointerDown`, `PointerMove`, `PointerUp` | `pointer_id`, `x`, `y` | Pointer contact in logical pixels. |
| `BackRequested` | — | System back gesture or button. |
| `Pause`, `Resume` | — | Frame updates pausing or resuming. |
| `Resize` | `width`, `height` | New drawable size in logical pixels. |
| `KeyDown`, `KeyUp` | `key`, `modifiers`, `repeat` | Portable key state change. |
| `NavigationInput` | `navigation`, `modifiers`, `input_device_id` | Semantic action from any device. |
| `TextInput` | `text` | Committed UTF-8 text. |
| `TextEditing` | `text`, `editing_start`, `editing_length` | Composition pre-edit update. |
| `TextInputShown`, `TextInputHidden` | — | Soft-keyboard visibility change. |

### Pointer identifiers

`pointer_id` is stable for one physical or logical contact: the same value
arrives on the `PointerDown`, `PointerMove`, and `PointerUp` events of that
contact. Values are never reused during the contact's lifetime. Positions
`x` and `y` are logical pixels in the coordinate space used for rendering,
so applications do not convert DPI or framebuffer scale themselves.

```cpp
void handle_event(const squared::application::Event& event) override
{
    if (event.type == squared::application::Event::Type::PointerDown) {
        active_pointer_ = event.pointer_id;
    } else if (event.type ==
                   squared::application::Event::Type::PointerUp &&
               event.pointer_id == active_pointer_) {
        last_position_ = {event.x, event.y};
        active_pointer_ = 0;
    }
}
```

### Keys, modifiers, and navigation

`Event::Key` covers the portable keys `unknown`, `left`, `right`, `up`,
`down`, `home`, `end`, `backspace`, `delete_key`, `enter`, `space`, `tab`,
and `escape`. `KeyModifiers` is a compact bit-field value object with
`contains()`, `set()`, and `empty()`; the `|` operator combines modifiers.

```cpp
using squared::application::Event;
using squared::application::KeyModifier;

if (event.key == Event::Key::space &&
    event.modifiers.contains(KeyModifier::control)) {
    pause_toggled_ = !pause_toggled_;
}
```

`Event::NavigationInput` carries a semantic action instead of a native button
constant: `left`, `right`, `up`, `down`, `next`, `previous`, `activate`, and
`cancel`. `input_device_id` identifies the source device and is stable while
the device stays connected. Platform adapters may produce these from game
controllers, keyboards, accessibility devices, or another source.

## Text input

Text input flows in two directions. Inbound, committed and composition
events arrive as `Event`:

- `TextInput` carries the committed text in `text` as UTF-8. Committed text
  is delivered whole; there is no separate insertion or deletion granularity.
  Applications that need character-by-character edits split the UTF-8 string
  themselves when the platform does not provide finer detail.
- `TextEditing` carries the current composition pre-edit in `text` (UTF-8)
  with `editing_start` and `editing_length` describing the span being edited
  in UTF-16 code units, matching platform composition reports. Use the UTF-16
  offsets to replace the correct range of the existing buffer; convert them
  to UTF-8 byte offsets if your buffer is UTF-8.
- `TextInputShown` and `TextInputHidden` report soft-keyboard visibility.

```cpp
if (event.type == squared::application::Event::Type::TextInput) {
    buffer_.append(event.text);
} else if (event.type == squared::application::Event::Type::TextEditing) {
    composition_start_ = event.editing_start;
    composition_length_ = event.editing_length;
}
```

Outbound, `TextInputService` lets application and GUI code show, reposition,
and hide the soft keyboard without depending on SDL, Android, or a window
system. The platform adapter implements the service and the application
injects it with `set_text_input_service()`; this is optional for
non-interactive applications. `TextInputRequest.area` is a rectangle in
logical pixels; `TextInputRequest.purpose` selects `normal`, `number`,
`email`, or `password` keyboard layouts. All service methods are called on
the application thread.

## Errors and failure behavior

`Application::create()` returns `false` when the application cannot enter its
event loop; the platform checks the result and aborts startup. All other
lifecycle hooks are `void`, and `quit_requested()` lets the application
itself request that the platform end the loop. No exception crosses the
boundary: the application reports failure through return values only.

## Threading

Every `Application` method and `TextInputService` method is called on the
application's main thread, the same thread that owns the platform event loop
and the graphics context. Applications must not call these from other
threads.

## Ownership and lifetime

- The platform adapter owns the window and rendering context and passes it
  by reference into `create()`, `surface_created()`, and `render()`. The
  application never owns or destroys it.
- `TextInputService` is injected non-owningly; the platform owns the service
  object, which must outlive the application.
- `Event` objects are ephemeral value aggregates valid only for the
  `handle_event()` call in which they arrive; copy any state you need.
- `KeyModifiers`, `TextInputArea`, and `TextInputRequest` are value objects
  that can be copied freely.

## Lua bindings

None of the types in this package have a Lua 5.4 binding.

## Related documentation

- Package payload: [Squared-Application.md](../../../packages/squared-application/content/docs/Squared-Application.md)
- Implementation details: [Squared Application — Developer Guide](../developer/squared-application/README.md)
- Documentation index: [Programmer documentation](../README.md)