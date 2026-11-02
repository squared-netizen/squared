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

### gl_enum_name

*const char * gl_enum_name(unsigned int value)*

### glsl_type_name

*const char * glsl_type_name(unsigned int type)*

### name_of

*const char * name_of(GlesErrorCode code)*

## Enums

| enum class BufferTarget |

--

| Vertex |
| Index |
| Uniform |


*Defined at [gles/src/buffer_target.cppm:9](../../../../gles/src/buffer_target.cppm#L9)*

| enum class BufferUsage |

--

| Static |
| Dynamic |
| Stream |


*Defined at [gles/src/buffer_usage.cppm:17](../../../../gles/src/buffer_usage.cppm#L17)*

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


*Defined at [gles/src/gles_error_code.cppm:9](../../../../gles/src/gles_error_code.cppm#L9)*

| enum class ShaderStage |

--

| Vertex |
| Fragment |


*Defined at [gles/src/shader_stage.cppm:9](../../../../gles/src/shader_stage.cppm#L9)*

| enum class TextureFormat |

--

| Rgba8 |
| Rgb8 |
| R8 |


*Defined at [gles/src/texture_format.cppm:9](../../../../gles/src/texture_format.cppm#L9)*

