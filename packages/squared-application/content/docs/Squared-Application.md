---
title: Squared Application
tags:
  - application
  - architecture
  - cpp
---

# Squared Application

Squared Application is a header-only module containing the platform-neutral
application lifecycle and event contracts.

`squared::application::Application` is implemented by developer application
code. A platform adapter owns operating-system and SDL integration, translates
native events into `squared::application::Event`, and calls the application
lifecycle without exposing SDL types through the public contract.

The interface covers creation, event handling, frame updates, rendering,
pause and resume, resize, graphics-surface availability, disposal, and an
application-requested shutdown query.

Graphics lifecycle callbacks have strict ordering. `create()` initializes
logical state once. After a usable context exists, the platform calls
`surface_created()`. Before rendering becomes unavailable it calls `pause()`
and then `surface_destroyed()`; this callback must not assume that graphics
commands remain legal. On foreground recovery, the platform restores the
context, calls `surface_created()` so resources can be recovered, resizes the
application, and finally calls `resume()`. `dispose()` releases logical state
once and may run while no rendering surface is available.

Keyboard and IME input remain portable in both directions. Platform adapters
translate native key, committed-text, and composition updates into `Event`.
Keyboard events carry `KeyModifiers`, a compact backend-neutral Shift,
Control, Alt, and Meta value. `Event::NavigationInput` carries semantic Left,
Right, Up, Down, Next, Previous, Activate, and Cancel actions with a portable
device identifier; platform adapters may produce them from game controllers,
keyboards, accessibility devices, or another source without exposing native
button constants.

They also implement `TextInputService`, which lets application or GUI code
start, reposition, and stop the soft keyboard without depending on SDL,
Android, or a particular window system. `Application::set_text_input_service`
is the injection point; it is optional for non-interactive applications.

The module has no implementation library or third-party dependency. Its CMake
target is `squared_application`, implemented as an `INTERFACE` target that
exports the headers and the C++20 requirement.
