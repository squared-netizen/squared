# struct JsonError

*Defined at [data/src/json_error.hpp:12](../../../../data/src/json_error.hpp#L12)*



**brief** Structured JSON failure information. 



## Members

public JsonErrorCode code

public int message

public int byte_offset

public int line

public int column



## Functions

### operator bool

*public bool operator bool()*

*Defined at [data/src/json_error.hpp:30](../../../../data/src/json_error.hpp#L30)*



**brief** Return whether this structure represents a failure. 



