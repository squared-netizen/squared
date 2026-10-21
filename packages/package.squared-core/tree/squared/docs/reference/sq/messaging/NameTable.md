# class NameTable

*Defined at [messaging/name_table.hpp:25](../../../../messaging/name_table.hpp#L25)*



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

### text

*public std::string_view text(Name name)*



**brief** A registered name's text; empty when it is not registered. 

### format

*public std::size_t format(Address address, std::span<char> out)*



**brief** Write `address` as text into `out`: `item[17].name`; a name not registered as `#` and 16 hex digits, then `[uid]`.

**return** The length written (no terminator), or 0 when `out` is too small.

### size

*public std::size_t size()*

*Defined at [messaging/name_table.hpp:57](../../../../messaging/name_table.hpp#L57)*



**brief** How many names are registered. 

### capacity

*public std::size_t capacity()*

*Defined at [messaging/name_table.hpp:63](../../../../messaging/name_table.hpp#L63)*



**brief** The storage's size. 

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


*Defined at [messaging/name_table.hpp:29](../../../../messaging/name_table.hpp#L29)*



**brief** What `add` did. 



