# struct CsiSequence

*Defined at [term/src/csi_sequence.cppm:18](../../../../../term/src/csi_sequence.cppm#L18)*

 One control sequence (ECMA-48's CSI ... final), as the parser hands it over: its parameters, a private marker (`?`, `>`, `<`, `=`, or 0) and up to two intermediates.

 Parameters are numbers up to 65535 (larger ones are clamped); an empty one is 0, which ECMA-48 makes "the default" for most functions. A parameter written after `:` is a sub-parameter of the one before it (ITU T.416's colours, `38:2::255:0:0`): bit i of `sub` is set when parameter i is one.



## Members

public array parameters

public std::uint32_t sub

public std::uint8_t count

public char private_marker

public array intermediates

public std::uint8_t intermediate_count

public char final

public static const std::size_t max_parameters



## Functions

### at

*public std::uint16_t at(std::size_t index, std::uint16_t fallback)*

*Defined at [term/src/csi_sequence.cppm:31](../../../../../term/src/csi_sequence.cppm#L31)*

 Parameter `index`, or `fallback` when it is missing or 0. 

### is_sub

*public bool is_sub(std::size_t index)*

*Defined at [term/src/csi_sequence.cppm:46](../../../../../term/src/csi_sequence.cppm#L46)*

### raw

*public std::uint16_t raw(std::size_t index)*

*Defined at [term/src/csi_sequence.cppm:41](../../../../../term/src/csi_sequence.cppm#L41)*

 Parameter `index` as written, 0 when missing. 

