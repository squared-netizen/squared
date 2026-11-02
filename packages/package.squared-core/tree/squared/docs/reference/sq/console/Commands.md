# class Commands

*Defined at [console/commands.cppm:19](../../../../console/commands.cppm#L19)*

## Functions

### add

*public AddResult add(const char * name, const char * help, Handler handler, void * context)*



**brief** Register a command; see AddResult. 

### all

*public std::span<const Command> all()*

*Defined at [console/commands.cppm:38](../../../../console/commands.cppm#L38)*

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



