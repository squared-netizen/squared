# gles &mdash; internals

Public GL object layer over GLES 3.0.

Programmer counterpart: [../programmer/gles.md](../programmer/gles.md)

- Public types: 16
- Translation units: 6

## Why it is public

It is the layer someone reaches for when they want a custom shader alongside
the widget set, and it is the layer OpenGL teaching tooling is built on. libGDX
exposes its equivalent (`glutils`) for the same reasons.

Being public costs an API surface that has to stay stable. That is paid for
deliberately, and it is why the types name things in squared's own enums rather
than raw `GLenum` &mdash; `BufferTarget::Index` survives a backend change in a
way `GL_ELEMENT_ARRAY_BUFFER` does not.

## Sizes

| Type | Bytes | Holds |
|---|---:|---|
| `VertexArray` | 4 | a name |
| `Shader` | 8 | a name and a stage |
| `Program` | 8 | a name |
| `Buffer` | 16 | a name, a target, a size |
| `Texture2D` | 16 | a name, dimensions, a format |

**No CPU-side mirror of anything.** That is the priority-1 decision at the
centre of the module. Retaining pixel data would make recovery after context
loss automatic and would double the resident cost of every texture in the
application. Recovery policy lives in `graphics2d::Texture`, where the caller
chooses per resource; this layer only offers what a policy needs.

## invalidate() is the non-obvious one

Every object has both `destroy()` and `invalidate()`, and calling the wrong one
after context loss produces corruption that reads as a driver bug.

`destroy()` calls `glDelete*`. After the context is gone, the name belonged to
a context that no longer exists: at best the call is a no-op, at worst the same
integer now names a different object in the new context and gets freed under
its owner.

`invalidate()` zeroes the name and touches no GL.

This is why the pair exists rather than a single `reset()`: the two cases look
identical at the call site and behave nothing alike, so they have different
names.

## Introspection is a query, not a cache

`Program::attributes()` and `Program::uniforms()` fill a caller-owned vector
and store nothing. A program drawn with sixty times a second is introspected
once, by a person or a tool, so the lists cost nothing until somebody asks.
Same pattern as `files::FileHandle::list()` and for the same reason.

`glGetActiveAttrib` reports an index, not a location, so each entry is followed
by a `glGetAttribLocation` call. They differ, and conflating them is a common
bug.

## Two driver traps handled here

**`GL_UNPACK_ALIGNMENT` is set to 1 before every upload.** GL unpacks rows on a
four-byte boundary by default, so an `R8` texture whose width is not a multiple
of four is read with a gap at the end of every row. The symptom is a font atlas
that comes out sheared, and it only appears at certain widths.

**A new texture gets sampling defaults immediately.** GL's default minification
filter is mipmap-based, so a texture with no mipmaps samples as black. Setting
`Linear`/`Linear`/`ClampToEdge` at creation means a freshly created texture
draws.

## Standard library deviations

None worth recording. No `std::function`, no `std::shared_ptr`, no
`std::unordered_map`. `std::string` appears only in `GlesError::message` and in
the introspection structs, both of which are diagnostic paths rather than per
frame. `std::vector` is used for driver log buffers, whose length only the
driver knows.

## Build wiring

The module is in `MODULES` only when `SQUARED_GRAPHICS_BACKEND` is `gles`,
because it cannot compile without the NDK's GL headers. The root `Makefile`
reads `local.mk` directly for that test: module Makefiles reach `local.mk`
through `build/common.mk`, but the root one does not include `common.mk`, so a
backend chosen in `local.mk` would otherwise select the right `context.cpp` and
the wrong module list.

## Not yet written

`Framebuffer` for render targets, and uniform buffer objects beyond
`BufferTarget::Uniform` existing as an enumerator. Neither has a second
concrete use yet; see [extension-policy.md](extension-policy.md).
