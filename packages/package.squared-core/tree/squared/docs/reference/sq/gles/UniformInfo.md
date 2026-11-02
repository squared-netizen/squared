# struct UniformInfo

*Defined at [gles/src/uniform_info.cppm:15](../../../../gles/src/uniform_info.cppm#L15)*



**brief** One uniform a linked program actually uses.

 Reported by Program::uniforms(). A uniform the shader declares but never reads is optimised away and will not appear here, and its location will be -1 — which is the usual reason a uniform "does not work".



## Members

public std::string name

public std::int32_t location

public std::uint32_t type

public std::int32_t size



