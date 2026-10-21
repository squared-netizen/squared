# class VertexArray

*Defined at [gles/vertex_array.hpp:24](../../../../gles/vertex_array.hpp#L24)*



**brief** One vertex array object, owned.

 A VAO records which buffers are bound and how their bytes are interpreted, so drawing is one bind instead of a dozen calls. 4 bytes; the state lives in the driver.

 GLES 3.0 guarantees VAOs. On GLES 2.0 they are an extension, which is part of why squared's floor is 3.0.



## Functions

### VertexArray

*public void VertexArray()*

*Defined at [gles/vertex_array.hpp:27](../../../../gles/vertex_array.hpp#L27)*

### VertexArray

*public void VertexArray(const VertexArray & )*

*Defined at [gles/vertex_array.hpp:30](../../../../gles/vertex_array.hpp#L30)*

### operator=

*public VertexArray & operator=(const VertexArray & )*

*Defined at [gles/vertex_array.hpp:31](../../../../gles/vertex_array.hpp#L31)*

### ~VertexArray

*public void ~VertexArray()*

### VertexArray

*public void VertexArray(VertexArray && other)*

### operator=

*public VertexArray & operator=(VertexArray && other)*

### create

*public GlesError create()*



**brief** Create the GL object. 

### set_vertices

*public GlesError set_vertices(const Buffer & vertices, std::span<const VertexAttribute> attributes)*



**brief** Record a vertex buffer and its attribute layout.

**vertices** Buffer holding the vertex data.

**attributes** One entry per attribute to enable.

**return** A no-error GlesError on success.

**note** This binds the VAO, binds the buffer, and enables each attribute. The bindings are recorded in the VAO, so the buffer does not have to stay bound afterwards — but it does have to stay alive.

### set_indices

*public GlesError set_indices(const Buffer & indices)*



**brief** Record an index buffer.

**indices** Buffer created with BufferTarget::Index.

**return** A no-error GlesError on success.

### bind

*public void bind()*



**brief** Bind this vertex array. 

### unbind

*public static void unbind()*



**brief** Unbind whatever vertex array is bound. 

### destroy

*public void destroy()*



**brief** Release the GL object. Safe to call more than once. 

### invalidate

*public void invalidate()*



**brief** Forget the GL name without deleting it, after context loss. 

### valid

*public bool valid()*

*Defined at [gles/vertex_array.hpp:71](../../../../gles/vertex_array.hpp#L71)*



**brief** Report whether this holds a GL object. 

### name

*public std::uint32_t name()*

*Defined at [gles/vertex_array.hpp:74](../../../../gles/vertex_array.hpp#L74)*



**brief** Return the raw GL name, or zero. 



