# namespace gles

## Records

* [AttributeInfo](AttributeInfo.md)
* [Buffer](Buffer.md)
* [GlesError](GlesError.md)
* [Program](Program.md)
* [Shader](Shader.md)
* [Texture2D](Texture2D.md)
* [UniformInfo](UniformInfo.md)
* [VertexArray](VertexArray.md)
* [VertexAttribute](VertexAttribute.md)


## Functions

### check_gl_errors

*GlesError check_gl_errors()*



**brief** Drain the GL error queue and report the first error found.

**return** A no-error GlesError when the queue was empty.

**note** GL accumulates errors and hands them back one at a time; leaving them queued makes the next unrelated check look like it failed. This drains the whole queue so a check means what it says.

### gl_enum_name

*const char * gl_enum_name(unsigned int value)*



**brief** Return a GL enum's identifier as text.

**value** Any GL enum, such as a type from Program::attributes().

**return** A static string such as "GL_FLOAT_VEC4", or "GL_?" when unknown.

### glsl_type_name

*const char * glsl_type_name(unsigned int type)*



**brief** Return the GLSL spelling of a GL type enum.

**type** A type as reported by Program::attributes() or uniforms().

**return** A static string such as "vec4", "mat3" or "sampler2D", or "?" when unknown.

**note** This is the name you would write in the shader source, which is the useful form when reading what the linker actually produced.

### name_of

*const char * name_of(GlesErrorCode code)*



**brief** Return the identifier of an error code as text.

**code** Code to name.

**return** A static string such as "CompileFailed"; never null.

**note** For logs and teaching output. The text is the enumerator name, not a sentence, so it is stable enough to grep for.

## Enums

| enum class BufferTarget |

--

| Vertex |
| Index |
| Uniform |


*Defined at [gles/src/buffer_target.hpp:9](../../../../gles/src/buffer_target.hpp#L9)*



**brief** What a buffer's contents are bound as. 

| enum class BufferUsage |

--

| Static |
| Dynamic |
| Stream |


*Defined at [gles/src/buffer_usage.hpp:17](../../../../gles/src/buffer_usage.hpp#L17)*



**brief** How often a buffer's contents are expected to change.

 This is a hint. The driver is free to ignore it, and getting it wrong costs performance rather than correctness. The distinction that matters in practice: Stream tells the driver you will overwrite the whole buffer every frame, which lets it hand you fresh memory instead of stalling until the GPU has finished reading the old contents.

| enum class GlesErrorCode |

--

| None |
| NoContext |
| OutOfMemory |
| CompileFailed |
| LinkFailed |
| InvalidArgument |
| Unsupported |
| GlError |


*Defined at [gles/src/gles_error_code.hpp:9](../../../../gles/src/gles_error_code.hpp#L9)*



**brief** Stable error categories produced by the GL object layer. 

| enum class ShaderStage |

--

| Vertex |
| Fragment |


*Defined at [gles/src/shader_stage.hpp:9](../../../../gles/src/shader_stage.hpp#L9)*



**brief** Which programmable stage a shader belongs to. 

| enum class TextureFormat |

--

| Rgba8 |
| Rgb8 |
| R8 |


*Defined at [gles/src/texture_format.hpp:9](../../../../gles/src/texture_format.hpp#L9)*



**brief** Pixel format of a texture's storage. 

