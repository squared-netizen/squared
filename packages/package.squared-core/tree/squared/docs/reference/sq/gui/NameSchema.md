# struct NameSchema

*Defined at [gui/layout_file.hpp:37](../../../../gui/layout_file.hpp#L37)*



**brief** `names.json`, read: every address, sorted by name, and the C++ screens and panes a layout may navigate to.



## Members

public vector names

public vector screens



## Functions

### find

*public const NameEntry * find(std::string_view name)*

### find_id

*public const NameEntry * find_id(messaging::Name name)*



**brief** By its id: what a telegram read from text carries. 

### has_screen

*public bool has_screen(std::string_view name)*



