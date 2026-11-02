# class Buffer

*Defined at [gles/buffer.cppm:17](../../../../gles/buffer.cppm#L17)*



**brief** One GL buffer object, owned.

 12 bytes: a name, a target and a size. Nothing is mirrored on the CPU, so a buffer costs what the GPU allocation costs and nothing more.



## Functions

### Buffer

*public void Buffer()*

*Defined at [gles/buffer.cppm:20](../../../../gles/buffer.cppm#L20)*

### Buffer

*public void Buffer(Buffer && other)*

### Buffer

*public void Buffer(const Buffer & )*

*Defined at [gles/buffer.cppm:23](../../../../gles/buffer.cppm#L23)*

### bind

*public void bind()*



**brief** Bind this buffer to its target. 

### create

*public GlesError create(BufferTarget target, BufferUsage usage, std::span<const std::byte> bytes, std::size_t size_bytes)*



**brief** Allocate storage and optionally fill it.

**target** What the buffer will be bound as.

**usage** Hint for how often the contents will change.

**bytes** Initial contents; may be empty to allocate uninitialised storage of size_bytes.

**size_bytes** Storage to allocate; ignored when bytes is non-empty.

**return** A no-error GlesError on success.

### destroy

*public void destroy()*



**brief** Release the GL object. Safe to call more than once. 

### invalidate

*public void invalidate()*



**brief** Forget the GL name without deleting it.

 Call this after the rendering context has been lost. The name belonged to a context that no longer exists; deleting it is at best a no-op and at worst frees an unrelated object in the new context. Every object in this module has this method for that reason, and getting it wrong produces corruption that looks like a driver bug.

### name

*public std::uint32_t name()*

*Defined at [gles/buffer.cppm:74](../../../../gles/buffer.cppm#L74)*



**brief** Return the raw GL name, or zero. For interop and teaching. 

### operator=

*public Buffer & operator=(Buffer && other)*

### operator=

*public Buffer & operator=(const Buffer & )*

*Defined at [gles/buffer.cppm:24](../../../../gles/buffer.cppm#L24)*

### size_bytes

*public std::size_t size_bytes()*

*Defined at [gles/buffer.cppm:80](../../../../gles/buffer.cppm#L80)*



**brief** Return the allocated size in bytes. 

### target

*public BufferTarget target()*

*Defined at [gles/buffer.cppm:77](../../../../gles/buffer.cppm#L77)*



**brief** Return the target this buffer was created for. 

### unbind

*public void unbind()*



**brief** Unbind whatever is bound to this buffer's target. 

### update

*public GlesError update(std::size_t offset_bytes, std::span<const std::byte> bytes)*



**brief** Replace part of the contents.

**offset_bytes** Byte offset to write at.

**bytes** Data to write; must fit within the allocated size.

**return** A no-error GlesError on success.

### valid

*public bool valid()*

*Defined at [gles/buffer.cppm:71](../../../../gles/buffer.cppm#L71)*



**brief** Report whether this buffer holds a GL object. 

### ~Buffer

*public void ~Buffer()*

