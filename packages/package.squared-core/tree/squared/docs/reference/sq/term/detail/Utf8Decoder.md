# class Utf8Decoder

*Defined at [term/src/utf8_decoder.cppm:15](../../../../../term/src/utf8_decoder.cppm#L15)*

 UTF-8, one byte at a time, as a stream delivers it: a character may be split between two reads. Well-formed per Unicode's table 3-7 (no overlong forms, no surrogates, nothing past U+10FFFF); each ill-formed part becomes one U+FFFD, the "maximal subpart" rule, as xterm and most terminals do. Four bytes of state.



## Members

public static const char32_t replacement



## Records

Result



## Functions

### pending

*public bool pending()*

*Defined at [term/src/utf8_decoder.cppm:54](../../../../../term/src/utf8_decoder.cppm#L54)*

 True between the bytes of one character. 

### push

*public Result push(std::uint8_t byte)*

*Defined at [term/src/utf8_decoder.cppm:35](../../../../../term/src/utf8_decoder.cppm#L35)*

### reset

*public void reset()*

*Defined at [term/src/utf8_decoder.cppm:56](../../../../../term/src/utf8_decoder.cppm#L56)*

## Enums

| enum class Step |

--

| more |
| code |
| code_then_again |


*Defined at [term/src/utf8_decoder.cppm:20](../../../../../term/src/utf8_decoder.cppm#L20)*

