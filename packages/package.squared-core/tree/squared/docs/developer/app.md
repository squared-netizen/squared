# app &mdash; internals

The platform-neutral boundary: eight types, no translation units, no
dependencies beyond `sq::graphics::Context` (forward declared).

Programmer counterpart: [../programmer/app.md](../programmer/app.md)

- Public types: 8
- Translation units: 0 &mdash; every type is a value type or a pure interface

## Layout

```
app/
  include/squared/app/   one header per public type
```

No `Makefile`, because there is nothing to compile. The headers are still
covered by `tools/check_headers.sh`, which discovers modules by globbing
`*/include` rather than listing them.

## The one deliberate rule deviation

`application.hpp` names `Event` and `TextInputService` only by reference, so
the tree's forward-declare-or-include rule would make both forward
declarations. It includes them instead, because `application.hpp` is also this
module's aggregate header: `Application` cannot be implemented without both,
and there is no free name left for a separate `<module>/<module>.hpp` umbrella
with the class already holding it.

`event.hpp` is likewise the `Event` struct rather than a group umbrella. Any
consumer that included it for `KeyModifier` still compiles, because `Event`
holds a `KeyModifiers` by value and therefore includes it transitively.

## Memory

`Event` carries a `std::string text` member for `TextInput` and `TextEditing`
events, which makes it 64 bytes and heap-capable, and it is passed by const
reference everywhere &mdash; so no copy is made on the dispatch path. The
platform adapter owns the one live `Event`. `KeyModifiers` is a single
`std::uint8_t`. `TextInputArea` is four floats.

Nothing in this module allocates on its own behalf.

## Free functions

`operator|(KeyModifier, KeyModifier)` lives in `key_modifiers.hpp` rather than
a header of its own: it builds a `KeyModifiers`, so that is the type it
belongs to.

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
