# struct ContextConfig

*Defined at [graphics/src/context_config.hpp:15](../../../../graphics/src/context_config.hpp#L15)*



**brief** What a Context needs to come up, supplied by the platform layer.

 The native window comes from the platform rather than from a title and a size, because that is the only form that generalises. Android hands the framework an ANativeWindow it did not create and cannot name; a desktop backend creates its own and wants a title. A config carries both, and each backend reads the fields that mean something to it.



## Members

public void * native_window

public const char * title

public int logical_width

public int logical_height



