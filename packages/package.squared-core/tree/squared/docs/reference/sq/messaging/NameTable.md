# class NameTable

*Defined at [messaging/name_table.cppm:30](../../../../messaging/name_table.cppm#L30)*

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

*Defined at [messaging/name_table.cppm:62](../../../../messaging/name_table.cppm#L62)*

### capacity

*public std::size_t capacity()*

*Defined at [messaging/name_table.cppm:68](../../../../messaging/name_table.cppm#L68)*

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



