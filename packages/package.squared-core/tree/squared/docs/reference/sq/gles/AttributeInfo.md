# struct AttributeInfo

*Defined at [gles/src/attribute_info.hpp:16](../../../../gles/src/attribute_info.hpp#L16)*



**brief** One vertex attribute a linked program actually uses.

 Reported by Program::attributes(). Worth knowing when reading a program: the linker removes attributes the shader never reads, so this list is what the GPU will really look for, not what the source appeared to declare.



## Members

public int name

public int location

public int type

public int size



