# struct LayoutNode

*Defined at [gui/layout.hpp:84](../../../../gui/layout.hpp#L84)*



**brief** One node of a layout: the widget it builds, where that sits in its cell, what it shows and what a tap sends. 32 bytes, read-only once made.

 Nodes are in document order: a table's, scroll's, stack's or list's nodes follow it, and `inside` counts them, theirs included. Words are offsets into the layout's text (`Layout::text`), 0 for none.



## Members

public Name bind

public std::uint16_t text

public std::uint16_t tap

public std::uint16_t bind_text

public std::uint16_t style

public std::uint16_t inside

public std::uint16_t numbers

public NodeType type

public Alignment align_x

public Alignment align_y

public std::uint8_t span

public float pad

public bool grow_x

public bool grow_y

public bool fill_x

public bool fill_y

public bool new_row

public bool vertical

public bool has_numbers

public bool sends_set

public ValueType value

public bool row_uid



