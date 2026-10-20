# struct ClipboardWriter

*Defined at [console/src/clipboard_writer.hpp:17](../../../../console/src/clipboard_writer.hpp#L17)*



**brief** Where the `copy` command writes: three functions and their context, so the Console needs no clipboard of its own and copies through no buffer.

 `begin` starts a clip and returns false when there is nowhere to write; `append` adds text, as often as needed; `end` hands the clip over and returns false if that failed. On Android the template's JNI writer builds a Java string piece by piece; a test collects into a string.



## Members

public void * context

public bool (*)(void *) noexcept begin

public void (*)(void *, int) noexcept append

public bool (*)(void *) noexcept end



