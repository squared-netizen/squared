# class Program

*Defined at [gles/program.hpp:26](../../../../gles/program.hpp#L26)*



**brief** One linked GL program, owned.

 8 bytes. Shaders are attached, linked and detached; nothing of them is kept.

 Attribute and uniform lists are queried on demand rather than cached, filling a caller-owned vector. A program that is drawn with a thousand times a second is introspected once, by a tool or a person, so the list costs nothing until somebody asks.



## Functions

### Program

*public void Program()*

*Defined at [gles/program.hpp:29](../../../../gles/program.hpp#L29)*

### Program

*public void Program(const Program & )*

*Defined at [gles/program.hpp:32](../../../../gles/program.hpp#L32)*

### operator=

*public Program & operator=(const Program & )*

*Defined at [gles/program.hpp:33](../../../../gles/program.hpp#L33)*

### ~Program

*public void ~Program()*

### Program

*public void Program(Program && other)*

### operator=

*public Program & operator=(Program && other)*

### link

*public GlesError link(const Shader & vertex, const Shader & fragment)*



**brief** Link a vertex and a fragment shader into a program.

**vertex** Compiled vertex shader.

**fragment** Compiled fragment shader.

**return** A no-error GlesError on success, or LinkFailed carrying the driver's log verbatim.

### build

*public GlesError build(int vertex_source, int fragment_source)*



**brief** Compile and link in one step.

**vertex_source** GLSL vertex source.

**fragment_source** GLSL fragment source.

**return** A no-error GlesError on success, or the first failure, carrying the log of whichever stage failed.

### use

*public void use()*



**brief** Make this program current. 

### uniform_location

*public int uniform_location(int name)*



**brief** Return the location of a uniform.

**name** Uniform name as written in the shader.

**return** The location, or -1 when the program has no such active uniform.

**note** -1 is not an error. A uniform the shader declares but never reads is removed by the linker, and that is the usual reason setting one appears to do nothing.

### set_float

*public void set_float(int location, float value)*



**brief** Set a float uniform. The program must be current. 

### set_int

*public void set_int(int location, int value)*



**brief** Set an int or sampler uniform. The program must be current. 

### set_vec2

*public void set_vec2(int location, float x, float y)*



**brief** Set a vec2 uniform. The program must be current. 

### set_vec4

*public void set_vec4(int location, float x, float y, float z, float w)*



**brief** Set a vec4 uniform. The program must be current. 

### set_mat4

*public void set_mat4(int location, const float * values)*



**brief** Set a mat4 uniform from 16 floats in column-major order.

**location** Uniform location.

**values** Sixteen floats; GL expects column-major, which is the order sq::math::Matrix4 already stores.

### attributes

*public GlesError attributes(int & out)*



**brief** List the vertex attributes this program actually uses.

**out** Cleared, then filled with one entry per active attribute.

**return** A no-error GlesError on success.

**note** The caller owns the vector so it can be reused. Nothing is cached.

### uniforms

*public GlesError uniforms(int & out)*



**brief** List the uniforms this program actually uses.

**out** Cleared, then filled with one entry per active uniform.

**return** A no-error GlesError on success.

### destroy

*public void destroy()*



**brief** Release the GL object. Safe to call more than once. 

### invalidate

*public void invalidate()*



**brief** Forget the GL name without deleting it, after context loss. 

### valid

*public bool valid()*

*Defined at [gles/program.hpp:111](../../../../gles/program.hpp#L111)*



**brief** Report whether this program holds a linked GL object. 

### name

*public int name()*

*Defined at [gles/program.hpp:114](../../../../gles/program.hpp#L114)*



**brief** Return the raw GL name, or zero. 



