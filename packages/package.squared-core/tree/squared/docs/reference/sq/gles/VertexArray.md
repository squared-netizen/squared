# class VertexArray

*Defined at [gles/vertex_array.cppm:22](../../../../gles/vertex_array.cppm#L22)*



**brief** One vertex array object, owned.

 A VAO records which buffers are bound and how their bytes are interpreted, so drawing is one bind instead of a dozen calls. 4 bytes; the state lives in the driver.

 GLES 3.0 guarantees VAOs. On GLES 2.0 they are an extension, which is part of why squared's floor is 3.0.



## Functions

### VertexArray

*public void VertexArray()*

*Defined at [gles/vertex_array.cppm:25](../../../../gles/vertex_array.cppm#L25)*

### VertexArray

*public void VertexArray(VertexArray && other)*

### VertexArray

*public void VertexArray(const VertexArray & )*

*Defined at [gles/vertex_array.cppm:28](../../../../gles/vertex_array.cppm#L28)*

### bind

*public void bind()*



**brief** Bind this vertex array. 

### create

*public GlesError create()*



**brief** Create the GL object. 

### destroy

*public void destroy()*



**brief** Release the GL object. Safe to call more than once. 

### invalidate

*public void invalidate()*



**brief** Forget the GL name without deleting it, after context loss. 

### name

*public std::uint32_t name()*

*Defined at [gles/vertex_array.cppm:72](../../../../gles/vertex_array.cppm#L72)*



**brief** Return the raw GL name, or zero. 

### operator=

*public VertexArray & operator=(VertexArray && other)*

### operator=

*public VertexArray & operator=(const VertexArray & )*

*Defined at [gles/vertex_array.cppm:29](../../../../gles/vertex_array.cppm#L29)*

### set_indices

*public GlesError set_indices(const Buffer & indices)*



**brief** Record an index buffer.

**indices** Buffer created with BufferTarget::Index.

**return** A no-error GlesError on success.

### set_vertices

*public GlesError set_vertices(const Buffer & vertices, std::span<const VertexAttribute> attributes)*



**brief** Record a vertex buffer and its attribute layout.

**vertices** Buffer holding the vertex data.

**attributes** One entry per attribute to enable.

**return** A no-error GlesError on success.

**note** This binds the VAO, binds the buffer, and enables each attribute. The bindings are recorded in the VAO, so the buffer does not have to stay bound afterwards — but it does have to stay alive.

### unbind

*public static void unbind()*



**brief** Unbind whatever vertex array is bound. 

### valid

*public bool valid()*

*Defined at [gles/vertex_array.cppm:69](../../../../gles/vertex_array.cppm#L69)*



**brief** Report whether this holds a GL object. 

### ~VertexArray

*public void ~VertexArray()*

