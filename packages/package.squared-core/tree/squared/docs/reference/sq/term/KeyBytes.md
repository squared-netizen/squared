# struct KeyBytes

*Defined at [term/key_bytes.cppm:15](../../../../term/key_bytes.cppm#L15)*



**brief** What a key sends to the program in the terminal: a few bytes, held by value (16 bytes, no allocation). Empty for a key that sends nothing.



## Members

public array bytes

public std::uint8_t size



## Functions

### empty

*public bool empty()*

*Defined at [term/key_bytes.cppm:21](../../../../term/key_bytes.cppm#L21)*

### view

*public std::string_view view()*

*Defined at [term/key_bytes.cppm:20](../../../../term/key_bytes.cppm#L20)*

