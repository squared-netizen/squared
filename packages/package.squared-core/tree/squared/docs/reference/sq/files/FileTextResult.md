# struct FileTextResult

*Defined at [files/src/file_text_result.cppm:16](../../../../files/src/file_text_result.cppm#L16)*



**brief** Text read from one file, or the failure that prevented it.

 The bytes are not validated or transcoded. A file that is not UTF-8 is returned as-is rather than rejected, because the callers that care about encoding know more about the file than this layer does.



## Members

public std::string text

public FileError error



## Functions

### operator bool

*public bool operator bool()*

*Defined at [files/src/file_text_result.cppm:25](../../../../files/src/file_text_result.cppm#L25)*



**brief** Return whether the read succeeded. 

