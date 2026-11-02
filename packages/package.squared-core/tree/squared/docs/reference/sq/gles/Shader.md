# class Shader

*Defined at [gles/shader.cppm:21](../../../../gles/shader.cppm#L21)*



**brief** One compiled shader stage, owned.

 A shader exists to be attached to a Program and then released; Program keeps nothing of it after linking. 8 bytes, no retained source and no retained log.

 The compile log is returned in the GlesError on failure rather than stored, because a successful shader's log is almost always empty and a failed one is about to be thrown away. Drivers put genuinely useful text there, so it is passed through verbatim.



## Functions

### Shader

*public void Shader()*

*Defined at [gles/shader.cppm:24](../../../../gles/shader.cppm#L24)*

### Shader

*public void Shader(Shader && other)*

### Shader

*public void Shader(const Shader & )*

*Defined at [gles/shader.cppm:27](../../../../gles/shader.cppm#L27)*

### compile

*public GlesError compile(ShaderStage stage, std::string_view source)*



**brief** Compile GLSL source for one stage.

**stage** Which programmable stage this is.

**source** GLSL source; the `#version` directive must be its first line, as GLSL requires.

**return** A no-error GlesError on success, or CompileFailed carrying the driver's log.

### destroy

*public void destroy()*



**brief** Release the GL object. Safe to call more than once. 

### invalidate

*public void invalidate()*



**brief** Forget the GL name without deleting it, after context loss. 

### name

*public std::uint32_t name()*

*Defined at [gles/shader.cppm:52](../../../../gles/shader.cppm#L52)*



**brief** Return the raw GL name, or zero. 

### operator=

*public Shader & operator=(Shader && other)*

### operator=

*public Shader & operator=(const Shader & )*

*Defined at [gles/shader.cppm:28](../../../../gles/shader.cppm#L28)*

### stage

*public ShaderStage stage()*

*Defined at [gles/shader.cppm:55](../../../../gles/shader.cppm#L55)*



**brief** Return the stage this shader was compiled for. 

### valid

*public bool valid()*

*Defined at [gles/shader.cppm:49](../../../../gles/shader.cppm#L49)*



**brief** Report whether this shader holds a compiled GL object. 

### ~Shader

*public void ~Shader()*

