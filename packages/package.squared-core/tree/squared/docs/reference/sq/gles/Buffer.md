# class Buffer

*Defined at [gles/buffer.hpp:20](../../../../gles/buffer.hpp#L20)*



**brief** One GL buffer object, owned.

 12 bytes: a name, a target and a size. Nothing is mirrored on the CPU, so a buffer costs what the GPU allocation costs and nothing more.



## Functions

### Buffer

*public void Buffer()*

*Defined at [gles/buffer.hpp:23](../../../../gles/buffer.hpp#L23)*

### Buffer

*public void Buffer(const Buffer & )*

*Defined at [gles/buffer.hpp:26](../../../../gles/buffer.hpp#L26)*

### operator=

*public Buffer & operator=(const Buffer & )*

*Defined at [gles/buffer.hpp:27](../../../../gles/buffer.hpp#L27)*

### ~Buffer

*public void ~Buffer()*

### Buffer

*public void Buffer(Buffer && other)*

### operator=

*public Buffer & operator=(Buffer && other)*

### create

*public GlesError create(BufferTarget target, BufferUsage usage, int bytes, int size_bytes)*



**brief** Allocate storage and optionally fill it.

**target** What the buffer will be bound as.

**usage** Hint for how often the contents will change.

**bytes** Initial contents; may be empty to allocate uninitialised storage of size_bytes.

**size_bytes** Storage to allocate; ignored when bytes is non-empty.

**return** A no-error GlesError on success.

### update

*public GlesError update(int offset_bytes, int bytes)*



**brief** Replace part of the contents.

**offset_bytes** Byte offset to write at.

**bytes** Data to write; must fit within the allocated size.

**return** A no-error GlesError on success.

### bind

*public void bind()*



**brief** Bind this buffer to its target. 

### unbind

*public void unbind()*



**brief** Unbind whatever is bound to this buffer's target. 

### destroy

*public void destroy()*



**brief** Release the GL object. Safe to call more than once. 

### invalidate

*public void invalidate()*



**brief** Forget the GL name without deleting it.

 Call this after the rendering context has been lost. The name belonged to a context that no longer exists; deleting it is at best a no-op and at worst frees an unrelated object in the new context. Every object in this module has this method for that reason, and getting it wrong produces corruption that looks like a driver bug.

### valid

*public bool valid()*

*Defined at [gles/buffer.hpp:74](../../../../gles/buffer.hpp#L74)*



**brief** Report whether this buffer holds a GL object. 

### name

*public int name()*

*Defined at [gles/buffer.hpp:77](../../../../gles/buffer.hpp#L77)*



**brief** Return the raw GL name, or zero. For interop and teaching. 

### target

*public BufferTarget target()*

*Defined at [gles/buffer.hpp:80](../../../../gles/buffer.hpp#L80)*



**brief** Return the target this buffer was created for. 

### size_bytes

*public int size_bytes()*

*Defined at [gles/buffer.hpp:83](../../../../gles/buffer.hpp#L83)*



**brief** Return the allocated size in bytes. 



