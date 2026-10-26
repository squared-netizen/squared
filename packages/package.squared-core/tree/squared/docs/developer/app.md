# app &mdash; internals

The platform-neutral boundary: twelve types, no translation units, no
dependencies beyond `sq::graphics::Context` (forward declared). Since 7.1 its
`Application` is implemented by `sq::App` ([stage.md](stage.md)), not by an
app.

Programmer counterpart: [../programmer/app.md](../programmer/app.md)

- Public types: 12
- Translation units: 0 &mdash; every type is a value type or a pure interface

## Layout

```
app/
  application.hpp event.hpp runtime.hpp safe_area.hpp
  src/   the nine supporting headers: key, modifiers, navigation, text input, and report
```

No `Makefile`, because there is nothing to compile. The headers are still
covered by `tools/check_headers.sh`, which finds every header at a module
root and under `src/` rather than listing modules. See
[file-layout.md](file-layout.md).

## The one deliberate rule deviation

`application.hpp` names `Event` and `TextInputService` only by reference, so
the tree's forward-declare-or-include rule would make both forward
declarations. It includes them instead, because `application.hpp` is also this
module's aggregate header: `Application` cannot be implemented without both,
and there is no free name left for a separate `<module>/<module>.hpp` umbrella
with the class already holding it.

`event.hpp` is likewise the `Event` struct rather than a group umbrella. It
includes `key.hpp`, `key_modifiers.hpp` and `navigation.hpp`, because `Event`
holds all three by value.

## The report sink and the lifecycle calls (7.1)

`Report` is a function pointer and a `void *` context, 16 bytes, with an
`operator()` that does nothing when empty: the platform gives one
(`set_report`, before `create()`), and the App and the Stage say each failure
through it, one line, with no allocation and no logging library of their own
(the framework above the porting layer names no Android header). Chosen over
`std::function` (heap and type erasure, `standard-library-deviations.md`) and
over a virtual sink interface (a vtable for one call).

`foreground()`, `background()` and `low_memory()` are Android's start, stop
and low-memory commands, which the glue already delivers: three virtual calls
with empty defaults, so an `Application` written before them (the demo's)
compiles and behaves as it did. They are events: nothing polls.

## Memory

`Event` carries a `std::string text` member for `TextInput` and `TextEditing`
events, which makes it 88 bytes (libstdc++, measured) and heap-capable, and it
is passed by const reference everywhere &mdash; so no copy is made on the
dispatch path. The platform adapter owns the one live `Event`. `KeyModifiers`
is one byte (`static_assert`ed). `TextInputArea` is four floats.

Nothing in this module allocates on its own behalf.

## One vocabulary for keys

**Before Sprint 3.1b** there were three key enums, two modifier types and two
navigation enums for the same things:
- keys: `app::Event::Key`, `gui::Key` (without `unknown`, so offset by one) and
  `gui::InputKey`;
- modifiers: `app::KeyModifiers` (a one-byte bitset driven by an
  `app::KeyModifier` flag enum) and `gui::InputModifiers` (four `bool`s), with
  `gui::KeyModifiers` an alias of the second;
- navigation: `app::Event::Navigation` and `gui::NavigationAction`.

`gui::Ui` and `gui::Widget` carried six switch statements converting between
them.

**Now** there is one of each, in this module, because it is the lowest layer
that both the platform entry file and the GUI see: `app::Key`, `app::KeyModifiers`
and `app::Navigation`. The GUI routes them unchanged, and the conversions are
gone (243 lines out of `ui.cpp` and `widget.cpp`). Pinned by
`gui/test/ui_keys_test.cpp`: every named key arrives as itself, with its
modifiers, and focus moves as it did.

**`KeyModifiers` is four one-bit `bool` fields** with default member
initializers.
- Priority 1: it stays one byte, where `gui::InputModifiers` was four, which
  takes `gui::InputEvent` from 88 to 80 bytes.
- Priority 2: callers read fields (`modifiers.shift`) and build with designated
  initializers (`KeyModifiers{ .shift = true }`), instead of `contains()`,
  `set()` and `operator|`.

`==` is defaulted, and `any()` replaces `empty()`.

## Safe area

`SafeArea` is four `std::int16_t` (8 bytes, `static_assert`ed), passed by const
reference. `Application::safe_area_changed()` is a defaulted virtual, so an
application that ignores it costs nothing and an old one still compiles.

**Measuring (Android).** NativeActivity has no call for the bar sizes, so the
platform entry file asks the Java side through JNI: `Activity.getWindow()` &rarr;
`getDecorView()` &rarr; `getRootWindowInsets()`, then
`getSystemWindowInsetLeft/Top/Right/Bottom()` and, where there is one, the
`DisplayCutout`'s safe insets, combined with `max`. Those `getSystemWindowInset*`
methods are deprecated from API 30 but present on every release squared
targets, and they avoid a second code path keyed on the SDK level. Every step
tolerates failure and falls back to zero, so a release where a call is
missing behaves as before.

The query runs after a drawable-size change and then about every 30th frame.
Polling is deliberate: the insets are not available when the first window
arrives and `APP_CMD_CONTENT_RECT_CHANGED` does not reliably report an
edge-to-edge window, so waiting for a notification can miss the only change
there is. Cost per query: about ten JNI calls, no allocation on the native side
(every local reference is deleted), and the thread is attached to the VM once
and detached at shutdown. The call is made from the native thread rather than
the UI thread; these are plain getters on state the framework already
published, which is how other native frameworks read them.

Known limitation: the JNI code lives in the seeded `sq_android/entry.cpp`, so
each generated project carries a copy. When the `app` module gets a compiled
library, it belongs there.

## Runtime

A non-owning bundle of three references: `graphics::Context`,
`files::FileSystem`, `assets::AssetManager`. Twenty-four bytes. The platform
layer owns the objects; `Runtime` only points at them.

**Admission rule.** A service belongs in `Runtime` only if its lifetime is the
whole process *and* only the platform layer can construct it. The context
needs the native window; the file system needs `AAssetManager` and the app's
storage paths; the asset manager needs the file system. All three pass. A
sprite batch, a skin or a UI fails the second test &mdash; the application can
build its own &mdash; and stays out.

That rule is the whole defence against this becoming a Service Locator. It is
not global, nothing reaches in by type, and it is passed once, explicitly, to
`create()`. The rule is what stops it growing into the object every subsystem
reaches into.

**Why references, not ownership.** `FileSystem` is polymorphic &mdash;
`PosixFileSystem` in Termux, `AndroidAssetFileSystem` on device. Owning it
would mean a `unique_ptr` and a heap allocation, or a template parameter on the
platform type. Referencing needs neither: the platform holds the concrete type
by value.

**Why only `create()`.** `render()` keeps taking the context alone. Passing the
whole bundle every frame invites reaching for the file system or the asset
cache at sixty hertz.

**Construction order.** In `entry.cpp`'s `Platform`, member order is
construction order: context, file system, asset manager, runtime, then the
application. The asset manager refers to the file system and the runtime to all
three, so each is declared after what it refers to, and the application comes
last so everything it is handed already exists.

The header forward declares all three and includes none, so it adds nothing to
a translation unit that only needs the type. An `App` that calls into a member
includes that member's header itself.

## `SQUARED_PLATFORM` must be passed, not just set

`pkg_squared_core.mk` chooses the platform and must pass it through the
`squared-core` recipe. For a while it was set to `android` and never passed:
the framework fell back to its default, `posix`, and every APK shipped a files
library without `AndroidAssetFileSystem` in it. Nothing failed at build time.

A selector that is set but not passed is silently ignored. Every one has to
appear in the recipe.
