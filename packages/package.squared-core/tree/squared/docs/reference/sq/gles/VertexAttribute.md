# struct VertexAttribute

*Defined at [gles/src/vertex_attribute.hpp:18](../../../../gles/src/vertex_attribute.hpp#L18)*



**brief** How one attribute is laid out inside a vertex buffer.

 The four numbers that confuse everyone the first time, named: `components` is how many floats this attribute has, `stride` is the size of one whole vertex, and `offset` is where this attribute starts inside that vertex. A position and a colour interleaved in one buffer share a stride and differ in offset.



## Members

public int location

public int components

public int stride

public int offset

public int instance_divisor



