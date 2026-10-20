# struct FileTextResult

*Defined at [files/src/file_text_result.hpp:17](../../../../files/src/file_text_result.hpp#L17)*



**brief** Text read from one file, or the failure that prevented it.

 The bytes are not validated or transcoded. A file that is not UTF-8 is returned as-is rather than rejected, because the callers that care about encoding know more about the file than this layer does.



## Members

public int text

public FileError error



## Functions

### operator bool

*public bool operator bool()*

*Defined at [files/src/file_text_result.hpp:26](../../../../files/src/file_text_result.hpp#L26)*



**brief** Return whether the read succeeded. 



