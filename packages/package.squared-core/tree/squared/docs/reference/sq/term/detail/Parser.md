# class Parser

*Defined at [term/src/parser.cppm:41](../../../../../term/src/parser.cppm#L41)*

 ECMA-48's syntax, from a byte stream: the state machine of Paul Williams' "A parser for DEC's ANSI-compatible video terminals" (vt100.net), on UTF-8. Bytes arrive as the program wrote them, in reads of any size; a sequence or a character split between two reads is put together. It owns no memory: the sequence being read and up to 256 bytes of an operating system command are its own members (about 360 bytes in all), so feeding it never allocates.

 The bytes 0x80..0xFF are UTF-8, never 8-bit controls (a C1 control comes as its code point, U+009B for CSI). A byte of 0x80 or more inside a sequence ends it, unread, and is read again as text. An operating system command longer than 256 bytes is dropped whole. DCS, SOS, PM and APC strings are read and ignored.



## Members

public static const std::size_t max_osc



## Functions

### feed

*public void feed(std::span<const std::byte> bytes, H & handler)*

*Defined at [term/src/parser.cppm:47](../../../../../term/src/parser.cppm#L47)*

### reset

*public void reset()*

*Defined at [term/src/parser.cppm:73](../../../../../term/src/parser.cppm#L73)*

 Back to text, anything half-read forgotten. 

