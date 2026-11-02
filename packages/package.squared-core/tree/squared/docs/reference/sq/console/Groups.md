# class Groups

*Defined at [console/groups.cppm:28](../../../../console/groups.cppm#L28)*

## Functions

### add

*public AddResult add(const char * group, const char * line)*



**brief** Add `line` to the group `group`, making the group if it is new. A line already in the group is not added twice (and is `added`).

### find

*public std::optional<std::size_t> find(std::string_view name)*



**brief** The number of the group named `name`, or nothing. 

### names

*public std::span<const char *const> names()*

*Defined at [console/groups.cppm:55](../../../../console/groups.cppm#L55)*

### lines

*public std::span<const GroupLine> lines()*

*Defined at [console/groups.cppm:61](../../../../console/groups.cppm#L61)*



## Enums

| enum class AddResult |

--

| added |
| groups_full |
| lines_full |
| invalid_name |


*Defined at [console/groups.cppm:32](../../../../console/groups.cppm#L32)*



