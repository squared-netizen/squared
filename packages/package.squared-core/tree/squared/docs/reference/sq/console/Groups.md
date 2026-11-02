# class Groups

*Defined at [console/groups.cppm:28](../../../../console/groups.cppm#L28)*



**brief** Named groups of command lines, for `dump`: `sys` runs every system command, `memory` the memory ones, and so on.

 A fixed table, like the command table: `group_slots` names (`CONSOLE_GROUPS`, 16) and `group_line_slots` lines among them (`CONSOLE_GROUP_LINES`, 128), held inline; adding never allocates. A line is a whole command line (`logcat 100`), so a group can pass arguments. Names and lines are kept as pointers: string literals are what they are meant to be.



## Functions

### add

*public AddResult add(const char * group, const char * line)*



**brief** Add `line` to the group `group`, making the group if it is new. A line already in the group is not added twice (and is `added`).

### find

*public std::optional<std::size_t> find(std::string_view name)*



**brief** The number of the group named `name`, or nothing. 

### lines

*public std::span<const GroupLine> lines()*

*Defined at [console/groups.cppm:61](../../../../console/groups.cppm#L61)*



**brief** Every line, in the order added; `group` says whose. 

### names

*public std::span<const char *const> names()*

*Defined at [console/groups.cppm:55](../../../../console/groups.cppm#L55)*



**brief** Every group's name, in the order made. 

## Enums

| enum class AddResult |

--

| added |
| groups_full |
| lines_full |
| invalid_name |


*Defined at [console/groups.cppm:32](../../../../console/groups.cppm#L32)*



**brief** Why add() refused. 

