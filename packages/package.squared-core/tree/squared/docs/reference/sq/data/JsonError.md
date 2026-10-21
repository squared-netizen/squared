# struct JsonError

*Defined at [data/src/json_error.hpp:12](../../../../data/src/json_error.hpp#L12)*



**brief** Structured JSON failure information. 



## Members

public JsonErrorCode code

public std::string message

public std::size_t byte_offset

public std::size_t line

public std::size_t column



## Functions

### operator bool

*public bool operator bool()*

*Defined at [data/src/json_error.hpp:30](../../../../data/src/json_error.hpp#L30)*



**brief** Return whether this structure represents a failure. 



