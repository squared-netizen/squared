# class Shader

*Defined at [gles/shader.hpp:23](../../../../gles/shader.hpp#L23)*



**brief** One compiled shader stage, owned.

 A shader exists to be attached to a Program and then released; Program keeps nothing of it after linking. 8 bytes, no retained source and no retained log.

 The compile log is returned in the GlesError on failure rather than stored, because a successful shader's log is almost always empty and a failed one is about to be thrown away. Drivers put genuinely useful text there, so it is passed through verbatim.



## Functions

### Shader

*public void Shader()*

*Defined at [gles/shader.hpp:26](../../../../gles/shader.hpp#L26)*

### Shader

*public void Shader(const Shader & )*

*Defined at [gles/shader.hpp:29](../../../../gles/shader.hpp#L29)*

### operator=

*public Shader & operator=(const Shader & )*

*Defined at [gles/shader.hpp:30](../../../../gles/shader.hpp#L30)*

### ~Shader

*public void ~Shader()*

### Shader

*public void Shader(Shader && other)*

### operator=

*public Shader & operator=(Shader && other)*

### compile

*public GlesError compile(ShaderStage stage, int source)*



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

### valid

*public bool valid()*

*Defined at [gles/shader.hpp:51](../../../../gles/shader.hpp#L51)*



**brief** Report whether this shader holds a compiled GL object. 

### name

*public int name()*

*Defined at [gles/shader.hpp:54](../../../../gles/shader.hpp#L54)*



**brief** Return the raw GL name, or zero. 

### stage

*public ShaderStage stage()*

*Defined at [gles/shader.hpp:57](../../../../gles/shader.hpp#L57)*



**brief** Return the stage this shader was compiled for. 



