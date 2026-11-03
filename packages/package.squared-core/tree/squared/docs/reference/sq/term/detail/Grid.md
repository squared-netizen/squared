# class Grid

*Defined at [term/src/grid.cppm:19](../../../../../term/src/grid.cppm#L19)*

 One screen of cells: rows of `columns` cells, in one block allocated when the size is set, with a table from the row on screen to the row in the block. Scrolling turns the table, two bytes a row moved, never the cells. Each row has a dirty bit (changed since the owner last drew it) and a wrapped bit (its text went on into the next row by autowrap).

 For 80 x 24: 23,040 bytes of cells, 48 of table, 8 of dirty bits, 24 of wrapped flags. Nothing is allocated but by `resize`.



## Functions

### Grid

*public void Grid()*

*Defined at [term/src/grid.cppm:22](../../../../../term/src/grid.cppm#L22)*

### clear

*public void clear(const Cell & fill)*

 Every row set to `fill`, and marked; none wrapped. 

### clear_dirty

*public void clear_dirty()*

### columns

*public std::uint16_t columns()*

*Defined at [term/src/grid.cppm:30](../../../../../term/src/grid.cppm#L30)*

### dirty

*public bool dirty(std::uint16_t row)*

*Defined at [term/src/grid.cppm:67](../../../../../term/src/grid.cppm#L67)*

### fill

*public void fill(std::uint16_t row, std::uint16_t first, std::uint16_t last, const Cell & fill)*

 Cells `first` to `last` of a row (both included) set to `fill`, the row marked. 

### mark

*public void mark(std::uint16_t row)*

*Defined at [term/src/grid.cppm:62](../../../../../term/src/grid.cppm#L62)*

### mark_all

*public void mark_all()*

### resize

*public void resize(std::uint16_t columns, std::uint16_t rows, bool keep, const Cell & fill)*

 The new size, blank (`fill`), or keeping what fits: the top left of the old one, row by row. `columns` and `rows` are at least 1.

### row

*public std::span<Cell> row(std::uint16_t row)*

*Defined at [term/src/grid.cppm:34](../../../../../term/src/grid.cppm#L34)*

 Row `row` on screen; `row` is below `rows()`. Does not mark it dirty. 

### row

*public std::span<const Cell> row(std::uint16_t row)*

*Defined at [term/src/grid.cppm:38](../../../../../term/src/grid.cppm#L38)*

### rows

*public std::uint16_t rows()*

*Defined at [term/src/grid.cppm:31](../../../../../term/src/grid.cppm#L31)*

### scroll_down

*public void scroll_down(std::uint16_t top, std::uint16_t bottom, std::uint16_t count, const Cell & fill)*

 The same, down: blank rows in at the top. 

### scroll_up

*public void scroll_up(std::uint16_t top, std::uint16_t bottom, std::uint16_t count, const Cell & fill)*

 Rows `top` to `bottom` (both included) moved up `count` rows, the rows that leave the top coming in at the bottom set to `fill`. Every row of the region is marked.

### set_wrapped

*public void set_wrapped(std::uint16_t row, bool wrapped)*

*Defined at [term/src/grid.cppm:77](../../../../../term/src/grid.cppm#L77)*

### wrapped

*public bool wrapped(std::uint16_t row)*

*Defined at [term/src/grid.cppm:73](../../../../../term/src/grid.cppm#L73)*

