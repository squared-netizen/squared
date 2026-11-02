# class Commands

*Defined at [console/commands.cppm:19](../../../../console/commands.cppm#L19)*



**brief** The console's fixed command table.

 `command_slots` entries (`CONSOLE_COMMANDS`, default 64), held inline; adding never allocates. Names and help texts are kept as pointers, so they must outlive the table: string literals are what they are meant to be. Lookup is a linear scan, which for a few dozen names typed by a person is the cheapest structure there is.



## Functions

### add

*public AddResult add(const char * name, const char * help, Handler handler, void * context)*



**brief** Register a command; see AddResult. 

### all

*public std::span<const Command> all()*

*Defined at [console/commands.cppm:38](../../../../console/commands.cppm#L38)*



**brief** Every registered command, in the order added. 

### find

*public const Command * find(std::string_view name)*



**brief** The command named `name`, or null. 

## Enums

| enum class AddResult |

--

| added |
| table_full |
| duplicate_name |
| invalid_name |


*Defined at [console/commands.cppm:23](../../../../console/commands.cppm#L23)*



**brief** Why add() refused. 

