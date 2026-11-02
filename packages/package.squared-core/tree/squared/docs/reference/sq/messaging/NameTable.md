# class NameTable

*Defined at [messaging/name_table.cppm:30](../../../../messaging/name_table.cppm#L30)*



**brief** The text of every registered name, for writing telegrams as text.

 Telegrams carry only a name's id; the dev log, replay files and saves write the text. The table lives in storage its owner gives it (the Telegraph's, sized by `TELEGRAPH_NAMES`), and allocates nothing: 16 bytes per name plus its text, so the default 8 KB holds about 250 names.

 Registration catches the one thing a 64-bit id can get wrong: two names with one id. Not thread-safe; register before the threads start.



## Functions

### NameTable

*public void NameTable(std::span<std::byte> storage)*



**brief** A table over `storage`, empty. The storage must outlive it, and is used as it is: no alignment needed.

### add

*public AddResult add(std::string_view text)*



**brief** Register a name. 

### capacity

*public std::size_t capacity()*

*Defined at [messaging/name_table.cppm:68](../../../../messaging/name_table.cppm#L68)*



**brief** The storage's size. 

### format

*public std::size_t format(Address address, std::span<char> out)*



**brief** Write `address` as text into `out`: `item[17].name`; a name not registered as `#` and 16 hex digits, then `[uid]`.

**return** The length written (no terminator), or 0 when `out` is too small.

### size

*public std::size_t size()*

*Defined at [messaging/name_table.cppm:62](../../../../messaging/name_table.cppm#L62)*



**brief** How many names are registered. 

### text

*public std::string_view text(Name name)*



**brief** A registered name's text; empty when it is not registered. 

### used

*public std::size_t used()*



**brief** Bytes in use, entries and text, of `capacity()`. 

## Enums

| enum class AddResult |

--

| added |
| known |
| invalid |
| collision |
| full |


*Defined at [messaging/name_table.cppm:34](../../../../messaging/name_table.cppm#L34)*



**brief** What `add` did. 

