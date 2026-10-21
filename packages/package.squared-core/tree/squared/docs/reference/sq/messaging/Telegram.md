# class Telegram

*Defined at [messaging/telegram.hpp:34](../../../../messaging/telegram.hpp#L34)*



**brief** One message: 32 bytes, copied whole through the Telegraph's rings.

 | Bytes | Field | |---|---| | 8 | the name's id | | 4 | the uid (0: none) | | 2 | the correlation, which pairs a `do` with the `event` answering it (0: none) | | 2 | the verb (2 bits) and the value type (4 bits); the other 10 bits are 0 | | 16 | the value: a bool, an integer, a double, up to four floats, or where its text or uid list is in the data area |

 Made by the factories, one per value type, so a telegram is always valid: the bytes a value does not use are zero, which makes two equal telegrams equal byte for byte. Apps send through the Telegraph with typed keys; this is what travels underneath.



## Functions

### address

*public Address address()*

*Defined at [messaging/telegram.hpp:55](../../../../messaging/telegram.hpp#L55)*

### empty

*public static Telegram empty(Verb verb, Address to)*



**brief** A telegram with no value (`event game.over`). 

### boolean

*public static Telegram boolean(Verb verb, Address to, bool value)*

### correlation

*public std::uint16_t correlation()*

*Defined at [messaging/telegram.hpp:56](../../../../messaging/telegram.hpp#L56)*

### integer

*public static Telegram integer(Verb verb, Address to, std::int64_t value)*

### number

*public static Telegram number(Verb verb, Address to, double value)*



**brief** A double; a NaN has no text form, so it is not a value (asserted). 

### floats

*public static Telegram floats(Verb verb, Address to, std::span<const float> values)*



**brief** Two to four floats (`vec2`..`vec4`); a NaN is not a value (asserted). 

### data

*public static Telegram data(Verb verb, Address to, ValueType type, DataRef where)*



**brief** Text or a uid list, by where it is in the data area. 

### with_correlation

*public Telegram with_correlation(std::uint16_t correlation)*



**brief** The same telegram, answering or awaiting `correlation`. 

### verb

*public Verb verb()*

### type

*public ValueType type()*

### as_boolean

*public bool as_boolean()*



**brief** The value, read as its type; meaningless for any other type. 

### as_integer

*public std::int64_t as_integer()*

### as_number

*public double as_number()*

### as_floats

*public std::array<float, 4> as_floats()*



**brief** The floats; `float_count(type())` of them are meaningful. 

### as_data

*public DataRef as_data()*

### valid

*public bool valid()*



**brief** Whether these 32 bytes are a telegram: a verb and a known type, the spare bits and unused value bytes zero, no NaN. Bytes that arrived from elsewhere (a file) are checked with it; the factories make only valid ones.



