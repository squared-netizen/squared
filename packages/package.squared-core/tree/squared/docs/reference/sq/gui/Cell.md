# class Cell

*Defined at [gui/cell.hpp:15](../../../../gui/cell.hpp#L15)*



**brief** Per-child constraints returned by Table::add for fluent configuration. 



## Functions

### column_span

*public Cell & column_span(std::size_t columns)*



**brief** Span multiple grid columns.

**columns** Number of grid columns covered by this cell; must be at least one.

**return** This cell for chaining.

### grow

*public Cell & grow()*



**brief** Stretch equally with other growing cells on both axes.

**return** This cell for chaining.

### grow_x

*public Cell & grow_x()*



**brief** Stretch horizontally with other growing cells.

**return** This cell for chaining.

### grow_y

*public Cell & grow_y()*



**brief** Stretch vertically with other growing cells.

**return** This cell for chaining.

### fill

*public Cell & fill()*



**brief** Expand to fill available cell space.

**return** This cell for chaining.

### fill_x

*public Cell & fill_x()*



**brief** Expand horizontally to fill available cell width.

**return** This cell for chaining.

### fill_y

*public Cell & fill_y()*



**brief** Expand vertically to fill available cell height.

**return** This cell for chaining.

### pad

*public Cell & pad(float value)*



**brief** Set uniform padding on all four sides.

**value** Padding in logical units; non-negative.

**return** This cell for chaining.

### pad

*public Cell & pad(Insets value)*



**brief** Set per-side padding.

**value** Insets in logical units; non-negative.

**return** This cell for chaining.

### align

*public Cell & align(Alignment horizontal, Alignment vertical)*



**brief** Set horizontal and vertical alignment within the cell.

**horizontal** start, center, or end alignment.

**vertical** start, center, or end alignment.

**return** This cell for chaining.

### widget

*public Widget & widget()*

*Defined at [gui/cell.hpp:88](../../../../gui/cell.hpp#L88)*



**brief** Access the child widget placed in this cell.

**return** Reference to the cell's child widget.



