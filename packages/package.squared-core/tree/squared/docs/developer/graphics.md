# graphics &mdash; internals

`Context` and `Color`: the whole rendering boundary above the backend.

Programmer counterpart: [../programmer/graphics.md](../programmer/graphics.md)

- Public types: 3
- Translation units: 1 per backend, one backend linked

## Link-time substitution

`Context` is a concrete `final` class with a fixed layout &mdash; a window
pointer, a native context pointer, pixel dimensions, a generation counter and a
preserved flag. It declares its members and defines none of them. A backend
supplies every definition.

That was the existing design, and it is the right one here: no vtable, no
indirection, one name at the seam, and the class was already shaped for it.
The cost is that a missing backend is a link error naming a private member
function, which is a terrible message. The null backend is the answer to that.

Choosing one is a make variable:

```sh
make SQUARED_GRAPHICS_BACKEND=null   # default
make SQUARED_GRAPHICS_BACKEND=gles
```

A new backend is a new directory under `graphics/src/` holding `context.cpp`.
No header changes, no other module changes, and `graphics/Makefile` names the
available ones when given a name that does not exist.

## The null backend

`graphics/src/null/context.cpp`. Every symbol resolves, nothing draws.

It is not a stub for its own sake. It buys three things:

- A project with no backend selected shows a black screen and runs, instead of
  failing to link. Debuggable rather than cryptic.
- `graphics2d`, `gui` and `scene2d` become linkable and testable on a machine
  with no GPU and no EGL. Before it, they could be compiled but never linked.
- It answers the same contract a real backend does, so a test written against
  it is a test of the contract: `create` succeeds, `valid()` turns true,
  `generation()` advances on every activation, `present()` returns true.

Nothing in it allocates.

## Two contract changes

**`present()` returns `[[nodiscard]] bool`.** It was `void`, which discarded
the only notice some devices give that the surface is gone. The failure mode is
an application frozen on its last frame after a phone call. `Context` already
carries `generation()` and `resources_preserved()`, so `present()` only has to
say *stop*; those two say *what changed*.

**`create()` takes a `ContextConfig`.** It was `create(title, width, height)`,
which cannot express an `ANativeWindow*` the platform hands you and did not
create. The config carries a `void*` native window alongside the title and
size, so no squared header includes a platform header, and each backend reads
the fields that mean something to it.

## The GLES backend

`graphics/src/gles/context.cpp`. EGL 1.4 and GLES 3.0, RGBA8 with a 16-bit
depth buffer and 8-bit stencil.

The display, config and surface are file-scope, and that is correct rather than
lazy: `EGL_DEFAULT_DISPLAY` is one display per process, and NativeActivity
gives a process one native window at a time. A second `Context` would be a
second window, which the platform does not offer. `Context`'s own members carry
what is per-instance: `window_` holds the `ANativeWindow*`, `native_context_`
holds the `EGLContext`.

**Buffer geometry is set from `EGL_NATIVE_VISUAL_ID` before creating the
surface.** Skipping that is the classic Android EGL bug: the surface is created
against a buffer format the config did not ask for, and it shows up as a failed
`eglMakeCurrent` or as colours that are wrong on some devices only.

**`resume()` tries the surviving context first.** If `eglMakeCurrent` binds the
old `EGLContext` to the new surface, every GPU object it held is still there
and `resources_preserved()` reports true. Only when that fails is a new context
created and the application told to rebuild. This is what makes backgrounding
cheap in the common case.

**`present()` reads `eglGetError()` on failure.** `EGL_BAD_SURFACE` and
`EGL_CONTEXT_LOST` both destroy the surface and return false, so the platform
layer's next `resume()` starts from a clean state.

`set_native_window()` exists because Android does not hand back the same
window: it destroys it on backgrounding and supplies a different one. `suspend`
and `resume` alone cannot express that, so the platform layer calls this
between them.

One deliberate deviation: the backend declares
`ANativeWindow_setBuffersGeometry` itself rather than including
`<android/native_window.h>`, which would pull in `hardware_buffer.h`,
`data_space.h` and `rect.h` for declarations nothing here uses. The function
has been public and stable since API 1, and a future change would be a compile
error at that line rather than a silent mismatch.

## Still missing

The GL object layer that `graphics2d` was written against. `SpriteBatch`
already declares `allocate_gpu_objects()` and `restore(context_preserved)`
against a layer that does not exist yet, and `Texture`, `TextureRegion`,
`TextureAtlas` and `AtlasRegion` have 55 declared member functions with no
definitions.
