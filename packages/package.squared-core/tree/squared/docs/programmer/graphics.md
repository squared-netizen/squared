# graphics

Backend-neutral colour and context types.

Every type below lives in its own header. Include exactly the ones a
translation unit names; `squared/graphics/graphics.hpp` pulls in all of them
and exists for convenience, not for use inside headers of your own.

Developer counterpart: [../developer/graphics.md](../developer/graphics.md)

| Type | Header | Purpose |
|---|---|---|
| `Color` | `squared/graphics/color.hpp` | Normalized four-component color used by Squared graphics APIs |
| `Context` | `squared/graphics/context.hpp` | Own the native window and rendering context selected at link time |

## Creating a context

The platform layer owns the `Context`; application code receives higher-level
objects and does not present the window itself.

```cpp
sq::graphics::Context context;
if (!context.create(sq::graphics::ContextConfig{
        .native_window = platform_window,   // ANativeWindow* on Android
        .title = "My Game",
        .logical_width = 1280,
        .logical_height = 720})) {
    return;
}
```

Leave `native_window` null on a backend that creates its own window.

## Per frame

```cpp
context.clear(background);
// ... draw ...
if (!context.present()) {
    // The surface is gone. Stop drawing and wait for the platform layer to
    // resume. generation() tells you whether GPU objects must be rebuilt.
}
```

**`present()` is `[[nodiscard]]` on purpose.** Ignoring it is how an
application ends up frozen on its last frame after a phone call.

## Surface loss

Android takes the rendering surface away when your application goes to the
background, and may or may not preserve GPU objects when it comes back.

```cpp
const std::uint64_t generation = context.generation();
if (!context.resume()) { return; }
if (context.generation() != generation && !context.resources_preserved()) {
    rebuild_gpu_objects();
}
```

## Choosing a backend

```sh
make SQUARED_GRAPHICS_BACKEND=null   # headless; the default
make SQUARED_GRAPHICS_BACKEND=gles   # EGL and GLES 3.0
```

The null backend draws nothing and succeeds at everything, so a project builds
and runs before a real backend exists, and `graphics2d`, `gui` and `scene2d`
can be tested without a GPU.
