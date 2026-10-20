# class Commands

*Defined at [console/commands.hpp:23](../../../../console/commands.hpp#L23)*



**brief** The console's fixed command table.

 `command_slots` entries (`CONSOLE_COMMANDS`, default 64), held inline; adding never allocates. Names and help texts are kept as pointers, so they must outlive the table: string literals are what they are meant to be. Lookup is a linear scan, which for a few dozen names typed by a person is the cheapest structure there is.



## Functions

### add

*public AddResult add(const char * name, const char * help, Handler handler, void * context)*



**brief** Register a command; see AddResult. 

### all

*public int all()*

*Defined at [console/commands.hpp:42](../../../../console/commands.hpp#L42)*



**brief** Every registered command, in the order added. 

### find

*public const Command * find(int name)*



**brief** The command named `name`, or null. 



## Enums

| enum class AddResult |

--

| added |
| table_full |
| duplicate_name |
| invalid_name |


*Defined at [console/commands.hpp:27](../../../../console/commands.hpp#L27)*



**brief** Why add() refused. 



