# namespace messaging

## Namespaces

* [detail](detail/index.md)
* [telegraph_size](telegraph_size/index.md)


## Records

* [Address](Address.md)
* [BoundEntry](BoundEntry.md)
* [CacheStats](CacheStats.md)
* [CachedValue](CachedValue.md)
* [DataRef](DataRef.md)
* [DoResult](DoResult.md)
* [Key](Key.md)
* [ListOutput](ListOutput.md)
* [ListSource](ListSource.md)
* [ListWindow](ListWindow.md)
* [ModelList](ModelList.md)
* [ModelRunner](ModelRunner.md)
* [ModelStats](ModelStats.md)
* [NameTableProbe](NameTableProbe.md)
* [NameTable](NameTable.md)
* [Name](Name.md)
* [OnUiThread](OnUiThread.md)
* [Received](Received.md)
* [RingStats](RingStats.md)
* [RowSpan](RowSpan.md)
* [TelegramBytes](TelegramBytes.md)
* [TelegramData](TelegramData.md)
* [TelegramRing](TelegramRing.md)
* [Telegram](Telegram.md)
* [TelegraphLine](TelegraphLine.md)
* [TelegraphUi](TelegraphUi.md)
* [Telegraph](Telegraph.md)
* [ValueCache](ValueCache.md)
* [Verbs](Verbs.md)
* [WaitStats](WaitStats.md)
* [Wake](Wake.md)


## Functions

### float_count

*std::size_t float_count(ValueType type)*

*Defined at [messaging/src/value_type.hpp:38](../../../../messaging/src/value_type.hpp#L38)*



**brief** How many floats a `vec2`, `vec3` or `vec4` holds; 0 for any other type. 

### list_source

*ListSource list_source(Source & source)*

*Defined at [messaging/model_list.hpp:51](../../../../messaging/model_list.hpp#L51)*



**brief** `source` as a `ListSource`, publishing through an `Out`. 

### name_hash

*std::uint64_t name_hash(std::string_view text, std::uint64_t seed)*

*Defined at [messaging/name.hpp:17](../../../../messaging/name.hpp#L17)*



**brief** 64-bit FNV-1a of a name's text, continuing from `seed`.

 The id a Name carries. Continuing from a seed lets a name be hashed in pieces, as `item[17].name` is: "item", then ".name", with nothing copied.

### operator<=>

*std::strong_ordering operator<=>(Name , Name )*

*Defined at [messaging/name.hpp:101](../../../../messaging/name.hpp#L101)*

### operator==

*bool operator==(DataRef , DataRef )*

*Defined at [messaging/src/data_ref.hpp:18](../../../../messaging/src/data_ref.hpp#L18)*

### operator==

*bool operator==(Name , Name )*

*Defined at [messaging/name.hpp:100](../../../../messaging/name.hpp#L100)*

### operator==

*bool operator==(RowSpan , RowSpan )*

*Defined at [messaging/list_window.hpp:18](../../../../messaging/list_window.hpp#L18)*

### operator==

*bool operator==(Verbs , Verbs )*

*Defined at [messaging/src/verb.hpp:44](../../../../messaging/src/verb.hpp#L44)*

### operator==

*bool operator==(const Address & , const Address & )*

*Defined at [messaging/address.hpp:24](../../../../messaging/address.hpp#L24)*

### operator==

*bool operator==(const Telegram & , const Telegram & )*

*Defined at [messaging/telegram.hpp:74](../../../../messaging/telegram.hpp#L74)*

### operator|

*Verbs operator|(Verbs a, Verbs b)*

*Defined at [messaging/src/verb.hpp:40](../../../../messaging/src/verb.hpp#L40)*

### parse_address

*std::optional<Address> parse_address(std::string_view text)*



**brief** An address from its text form, with no name table: the name is hashed.

 `player.hp`, `item[17].name`, `inventory[20]`: the uid in brackets after the first segment, in decimal. `#` and 16 hex digits stand for a name's id written without its text, with the uid after it (`#0123456789abcdef[17]`), which is how an unregistered name is written. `[0]` reads as no uid.

**return** The address, or nothing when `text` is not one (not a valid name, a uid past 4294967295, brackets anywhere else).

### read_telegram

*std::optional<Telegram> read_telegram(std::string_view line, TelegramData & data)*



**brief** A telegram from its line, with no table (names are hashed). Text and uid lists go into `data`.

 Reads more than the canonical form (more spaces, `[0]`, `1.50`), and writing what it read gives the canonical line.

**return** The telegram, or nothing when the line is not one, or `data` is full.

### valid_name

*bool valid_name(std::string_view text)*

*Defined at [messaging/name.hpp:35](../../../../messaging/name.hpp#L35)*



**brief** Whether `text` is a name: dot-separated segments of `a`-`z`, `0`-`9` and `_`, none empty, at most 255 bytes in all (`player.hp`, `item.name`).

 One spelling per name, so one id per name: no capitals, no spaces. Which object a telegram is about never goes in the name; that is the uid.

### value_type_of

*ValueType value_type_of()*

*Defined at [messaging/src/value_type.hpp:69](../../../../messaging/src/value_type.hpp#L69)*



**brief** The value type a `Key`'s C++ type means. 

### verb_text

*std::string_view verb_text(Verb verb)*



**brief** The text of a verb: `set`, `do`, `state`, `event`. 

### write_telegram

*std::size_t write_telegram(const Telegram & telegram, std::span<const std::byte> data, const NameTable & names, std::span<char> out)*



**brief** A telegram as its one canonical line of text: what the dev log prints, and what replay files and saves hold.

 ```text set settings.sound false do file.open "notes.txt" @12 state item[17].name "fur coat" state player.pos (1.5 2) event game.over @12 ```

 `<verb> <address> <value>`, then ` <correlation>` when it is not 0. The value: `true`/`false`; an integer in decimal; a double in the shortest form that reads back to the same bits, always with a `.` or an exponent so it is not an integer (`2.0`, `1e+21`, `inf`); two to four floats in parentheses; text in double quotes with `\`, `"`, `**n** `, `**<not a builtin command>** ` and `**<not a builtin command>** ` for any other byte below 0x20 or 0x7F (other bytes as they are); uids in brackets. The same telegram always gives the same line, on every device: no locale.



**data** The data area the telegram's text or uid list is in.

**return** The length written, or 0 when `out` is too small, the telegram is not valid, or its data is outside `data`.

### write_value

*std::size_t write_value(const Telegram & telegram, std::span<const std::byte> data, std::span<char> out)*



**brief** Only the value of a telegram, as `write_telegram` writes it (`42`, `2.0`, `(1.5 2)`, `"fur coat"`): what a label bound to it shows, text aside, which it shows as it is.

**return** The length written; 0 when `out` is too small, the telegram has no value or is not valid, or its data is outside `data`.

## Enums

| enum class CacheResult |

--

| stored |
| dropped |
| too_large |


*Defined at [messaging/value_cache.hpp:15](../../../../messaging/value_cache.hpp#L15)*



**brief** What `ValueCache::store` did. 

| enum class RingResult |

--

| sent |
| full |
| too_large |


*Defined at [messaging/telegram_ring.hpp:15](../../../../messaging/telegram_ring.hpp#L15)*



**brief** What `TelegramRing::push` did. 

| enum class SendResult |

--

| sent |
| pending |
| full |
| too_large |


*Defined at [messaging/telegraph.hpp:33](../../../../messaging/telegraph.hpp#L33)*



**brief** What a send did. 

| enum class TapDirection |

--

| to_model |
| to_ui |


*Defined at [messaging/telegraph.hpp:68](../../../../messaging/telegraph.hpp#L68)*



**brief** Which way a telegram crossed, for a tap. 

| enum class ValueType |

--

| none |
| boolean |
| integer |
| number |
| vec2 |
| vec3 |
| vec4 |
| text |
| uids |


*Defined at [messaging/src/value_type.hpp:25](../../../../messaging/src/value_type.hpp#L25)*



**brief** What a telegram's value is, in four bits.

 | Type | Held | Written | |---|---|---| | `none` | nothing | (nothing) | | `boolean` | in the telegram | `true`, `false` | | `integer` | in the telegram, 64-bit signed | `42`, `-7` | | `number` | in the telegram, a double | `1.5`, `2.0`, `1e+21`, `inf` | | `vec2`, `vec3`, `vec4` | in the telegram, floats | `(1.5 2 0 1)` | | `text` | in the data area | `"notes.txt"` | | `uids` | in the data area, 32-bit each | `[17 4 22]` |

| enum class Verb |

--

| set |
| do_ |
| state |
| event |


*Defined at [messaging/src/verb.hpp:18](../../../../messaging/src/verb.hpp#L18)*



**brief** What a telegram does, in two bits.

 | Verb | Direction | Means | |---|---|---| | `set` | UI to model | the user changed a value | | `do_` | UI to model | an intent (`do file.open "notes.txt"`) | | `state` | model to UI | a value is now this | | `event` | model to UI | something happened |

