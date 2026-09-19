# gles

The GL object layer: `Buffer`, `Shader`, `Program`, `VertexArray`,
`Texture2D`. RAII wrappers over GLES 3.0, public on purpose.

Developer counterpart: [../developer/gles.md](../developer/gles.md)

| Type | Header | Purpose |
|---|---|---|
| `Buffer` | `squared/gles/buffer.hpp` | one GL buffer object, owned |
| `BufferTarget` | `squared/gles/buffer_target.hpp` | vertex, index or uniform |
| `BufferUsage` | `squared/gles/buffer_usage.hpp` | static, dynamic or stream hint |
| `Shader` | `squared/gles/shader.hpp` | one compiled stage, owned |
| `ShaderStage` | `squared/gles/shader_stage.hpp` | vertex or fragment |
| `Program` | `squared/gles/program.hpp` | one linked program, owned |
| `AttributeInfo` | `squared/gles/attribute_info.hpp` | an attribute the linker kept |
| `UniformInfo` | `squared/gles/uniform_info.hpp` | a uniform the linker kept |
| `VertexArray` | `squared/gles/vertex_array.hpp` | one VAO, owned |
| `VertexAttribute` | `squared/gles/vertex_attribute.hpp` | one attribute's layout |
| `Texture2D` | `squared/gles/texture2d.hpp` | one 2D texture, owned |
| `TextureFormat` | `squared/gles/texture_format.hpp` | Rgba8, Rgb8 or R8 |
| `TextureFilter` | `squared/gles/texture_filter.hpp` | nearest or linear |
| `TextureWrap` | `squared/gles/texture_wrap.hpp` | clamp, repeat or mirrored |
| `GlesError` | `squared/gles/gles_error.hpp` | structured failure, with driver logs |
| `GlesErrorCode` | `squared/gles/gles_error_code.hpp` | stable error categories |

## A triangle

```cpp
sq::gles::Program program;
if (auto error = program.build(vertex_source, fragment_source)) {
    log(error.message);            // the driver's own compile or link log
    return;
}

sq::gles::Buffer vertices;
if (auto error = vertices.create(
        sq::gles::BufferTarget::Vertex,
        sq::gles::BufferUsage::Static,
        std::as_bytes(std::span{data}))) { return; }

sq::gles::VertexArray array;
if (auto error = array.create()) { return; }

const sq::gles::VertexAttribute layout[] = {
    {.location = 0, .components = 2, .stride = 16, .offset = 0},
    {.location = 1, .components = 2, .stride = 16, .offset = 8}
};
if (auto error = array.set_vertices(vertices, layout)) { return; }

program.use();
array.bind();
glDrawArrays(GL_TRIANGLES, 0, 3);
```

`stride` is the size of one whole vertex; `offset` is where this attribute
starts inside it. Two attributes interleaved in one buffer share a stride and
differ in offset.

## Seeing what the program actually is

The linker removes anything a shader declares but never reads. That is the
usual reason a uniform "does not work", and these two queries make it visible:

```cpp
std::vector<sq::gles::UniformInfo> uniforms;
if (!program.uniforms(uniforms)) {
    for (const auto& uniform : uniforms) {
        std::printf("%-20s %-10s loc %d\n",
                    uniform.name.c_str(),
                    sq::gles::glsl_type_name(uniform.type),
                    uniform.location);
    }
}
```

`glsl_type_name()` gives the spelling you would write in the shader &mdash;
`vec4`, `mat3`, `sampler2D`. `gl_enum_name()` gives the `GL_` identifier when
you want the raw form.

Nothing is cached. Both fill a caller-owned vector, so a program that is drawn
with a thousand times a second costs nothing for lists that are read once.

## Errors

Every failing call returns a `GlesError`. Nothing throws.

For `CompileFailed` and `LinkFailed`, `message` is **the driver's own log,
verbatim**. Drivers say genuinely useful things there and squared does not
paraphrase them.

```cpp
if (auto error = shader.compile(stage, source)) {
    std::printf("%s: %s\n", sq::gles::name_of(error.code),
                error.message.c_str());
}
```

`check_gl_errors()` drains the GL error queue and reports the first failure.
Draining matters: GL hands errors back one at a time, and leaving any queued
makes the next unrelated check report a failure that already happened.

## After context loss

Every object has `invalidate()`, and you must call it rather than `destroy()`:

```cpp
if (!context.resume()) { return; }
if (!context.resources_preserved()) {
    texture.invalidate();          // NOT destroy()
    buffer.invalidate();
    program.invalidate();
    rebuild_everything();
}
```

`destroy()` calls `glDelete*` on a name that belonged to a context which no
longer exists. At best that is a no-op; at worst it frees an unrelated object
in the new context, and the corruption looks like a driver bug. `invalidate()`
forgets the name without touching GL.

## Building

`gles` is built only when the graphics backend is `gles`, because it needs the
NDK's GL headers:

```make
# local.mk
SQUARED_GRAPHICS_BACKEND := gles
SQUARED_EXTERNAL_INCLUDES += -idirafter $(SQ_NDK_INC)
```

A null-backend build skips the module entirely, so the rest of squared still
builds on a machine with no GL headers at all.
