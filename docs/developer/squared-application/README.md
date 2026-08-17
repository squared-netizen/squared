# Squared Application — Developer Guide

Squared Application defines the portable application lifecycle and the
framework event boundary. The package is entirely header-only: everything
compiles into the declaring translation unit, there is no `.cpp` file, and
the CMake target is an `INTERFACE` library. A platform adapter (for example
`examples/gui-showcase/android/platform/sdl_main.cpp`) implements the
binding between SDL and this boundary.

- Programmer counterpart: [Squared Application — Programmer Guide](../programmer/squared-application/README.md)
- Package payload: [Squared-Application.md](../../packages/squared-application/content/docs/Squared-Application.md)
- Documentation index: [Developer documentation](../README.md)
- Diagrams:
  - [Application lifecycle and callback order](lifecycle-and-callbacks.dot)
  - [Event and text-input flow](event-textinput-flow.dot)

## Dependency boundary

The manifest declares `module.requires` as `[]`: the package depends on no
other Squared module and no third-party library. The CMake
`modules/squared-application/CMakeLists.txt` creates `squared_application` as
an `INTERFACE` target with only `cxx_std_20` and the `include/` directory.

The public headers include only `squared/application/event.hpp` and
`squared/application/text_input.hpp`, plus `<chrono>`. The only foreign type
is a forward declaration of `squared::graphics::Context`, which the platform
adapter owns; the header never includes it. SDL, OpenGL, Android, GUI, and
backend headers never appear in the public headers, so a pure
`#include <squared/application/application.hpp>` translation unit — such as
`tests/application-boundary-test.cpp` — compiles with only that include path.

## Architecture

| Component | Source | Responsibility |
| --- | --- | --- |
| `Application` | `include/squared/application/application.hpp` | Lifecycle interface consumed by the platform adapter. |
| `Event`, `Event::Type`, `Event::Key`, `Event::Navigation` | `include/squared/application/event.hpp` | Compact tagged event aggregate and portable enums. |
| `KeyModifier`, `KeyModifiers` | `include/squared/application/event.hpp` | Modifier flags and their bit-field set. |
| `TextInputPurpose`, `TextInputArea`, `TextInputRequest` | `include/squared/application/text_input.hpp` | Soft-keyboard request value objects. |
| `TextInputService` | `include/squared/application/text_input.hpp` | Application-to-platform soft-keyboard contract. |

There is no implementation directory. The boundary is a set of declarations;
behavior is entirely in developer applications and platform adapters.

## Ownership and threading

- The platform adapter owns process setup and the `squared::graphics::Context`,
  which is passed into `create()`, `surface_created()`, and `render()` by
  reference for the lifetime of the call.
- The adapter also owns the `TextInputService` implementation and injects it
  non-owningly through `set_text_input_service()`. The service must outlive
  the application.
- `Event` objects are temporaries: the adapter's `translate_event` (see
  `sdl_main.cpp`) creates one per native event, `handle_event` consumes it,
  and it is destroyed at the end of the poll iteration.
- All `Application` and `TextInputService` methods run on the single
  application thread; nothing in the boundary is thread-safe, and nothing
  needs to be, because the loop and the GL context are owned by that thread.

## Invariants and failure behavior

- `create()` must be called first and returns `false` to abort startup; after
  a successful `create()` the adapter must still call `surface_created()`
  before the first render.
- `update(delta)` receives elapsed domain time in nanoseconds; the example
  adapter clamps it to `0.1` seconds to avoid large jumps after frames.
- `surface_destroyed()` must not execute graphics commands; GPU resources are
  only valid again after `surface_created()` following a successful
  `graphics.resume()`.
- `Pause` events stop the frame loop; only `Resume` after context restoration
  restarts it. A failed `graphics.resume()` ends the loop (`running = false`),
  because no rendering surface exists.
- `TextEditing.editing_start`/`editing_length` are UTF-16 code-unit offsets,
  while `Event.text` is UTF-8; adapters must copy the platform values
  verbatim and applications convert when they need UTF-8 byte offsets.
- `TextInput` text arrives committed and whole; there is no per-insertion or
  per-deletion event.

## Data structures and complexity

- `Event` is a single aggregate `struct` whose members are `type`, the pointer
  fields, the resize fields, the key name, a `KeyModifiers`, the navigation
  action, `input_device_id`, `repeat`, `text` (`std::string`), and the two
  editing offsets. All members except `text` are scalar or a small bit-field,
  so construction and copying are O(1) apart from the string, which moves or
  copies its buffer on demand. Gui and application code treats the aggregate
  as immutable per event.
- `KeyModifiers` stores one `std::uint8_t` bit field; `contains`, `set`, and
  `empty` are O(1). The `operator|` overload builds a temporary and is
  `constexpr`.
- The platform adapter uses `std::unordered_map<SDL_JoystickID, SDL_GameController*>`
  to track controllers keyed by stable SDL joystick instance IDs; that is
  adapter-owned state, not part of the portable package.

## Algorithms and execution order

The adapter loop in `sdl_main.cpp` is the reference sequencing:

1. Create the rendering context, then the application, inject the text-input
   service, call `create(graphics)`, then `surface_created(graphics)`, then
   `resize(pixel_width, pixel_height)`.
2. Poll `SDL_PollEvent`. For each native event: update controllers, translate
   with `translate_event`, then classify. Lifecycle events (`Pause`,
   `Resume`, `Resize`, `QuitRequested`) are handled by the adapter before
   delivery; every other event is forwarded verbatim.
3. `Pause`: call `application->pause()`, stop text input, and call
   `surface_destroyed()` once; mark the surface unavailable.
4. `Resume`: call `graphics.resume()`, then `surface_created(graphics)`,
   `resize(...)`, and `application->resume()`; restart the timing counter.
5. When not paused, compute delta, call `update(delta)`, `render(graphics)`,
   and `graphics.present()`.
6. On exit: stop text input, `surface_destroyed()` if still available,
   `dispose()`, destroy the application, `graphics.destroy()`.

Event translation is a narrowing switch: native key codes map to
`Event::Key`, modifiers to `KeyModifiers`, controller buttons to
`Event::Navigation`, finger and mouse positions to pointer events, and
`SDL_TEXTINPUT`/`SDL_TEXTEDITING` to text events. The Android back key closes
text input first and otherwise becomes `BackRequested`. Pointer identifiers
come from `event.tfinger.fingerId` or `event.button.which`, the platform's
per-contact IDs, preserving the stable-identifier contract.

## Design patterns

- **Observer** — the platform adapter observes native SDL events (the
  subject) and notifies `Application::handle_event` (the observer). The
  textbook form notifies many observers with associated events; the boundary
  uses a single observer per application and pushes a portable event value,
  which keeps the coupling to exactly one sink.
- **Strategy** — `TextInputService` encapsulates soft-keyboard behavior and
  lets the platform substitute its own implementation behind one interface;
  application and GUI code depend only on the abstraction. This is the
  textbook pattern (interface + swappable implementation) with a deliberate
  deviation: the strategy is injected through `Application` instead of a
  dedicated constructor, so existing applications can opt in lazily.
- **Facade** — `Application` and the event types present a small portable
  surface that hides SDL event details, bitmap state, and Android specifics
  from application code.

Alternatives rejected: a deep class hierarchy of event subclasses (rejected
for value semantics and allocation-free copying of a single aggregate); a
portable dynamic input-device abstraction inside the package (rejected — the
package must stay free of any platform dependency and the adapter already
provides `input_device_id`).

## Limitations and technical debt

- **Sparse event coverage**: the key set is intentionally small
  (`left`, `right`, `up`, `down`, `home`, `end`, `backspace`, `delete_key`,
  `enter`, `space`, `tab`, `escape`, `unknown`). Typing applications must
  rely on `TextInput` committed text rather than per-`KeyDown` chars.
- **UTF-16 editing offsets**: `editing_start`/`editing_length` are in UTF-16
  code units to mirror platform composition reports, while payload text is
  UTF-8; toolkits holding UTF-8 buffers must convert.
- **No clipboard service**: `TODO.md` records clipboard boundaries for
  text-editing clients as unfinished work.
- **No device-added/removed notifications**: `TODO.md` lists explicit
  device notifications as future work; the example adapter hides controller
  plugging behind its own `SDL_GameController` map.
- **Single-window lifecycle**: `TODO.md` flags multi-window coverage review;
  the current boundary assumes one drawable and one event loop.
- The package is pure interface; correctness of ordering depends on the
  adapter following the documented sequence, so any future adapter must copy
  the `sdl_main.cpp` ordering.