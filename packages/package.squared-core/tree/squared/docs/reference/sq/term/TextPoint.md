# struct TextPoint

*Defined at [term/text_point.cppm:13](../../../../term/text_point.cppm#L13)*



**brief** A place in a terminal's text: a line, numbered as the terminal numbers them (`Terminal::first_line`, `screen_line`: a line keeps its number as more scroll off), and a column. Ordered by line, then column.



## Members

public std::uint64_t line

public std::uint16_t column



