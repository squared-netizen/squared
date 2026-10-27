# class Table

*Defined at [gui/table.hpp:20](../../../../gui/table.hpp#L20)*



**brief** Grid layout with libGDX-style rows and chainable cell constraints.



Inherits from Widget



## Functions

### release_kept

*public static void release_kept()*



**brief** Free the working sizes every table's layout borrows (one set a nesting depth, kept so a layout allocates nothing once warm): on low memory. The next layout makes them again. Nothing while one is in use.

### kept_count

*public static std::size_t kept_count()*



**brief** How many sets of working sizes are kept: one a nesting depth laid out. 

### add

*public Cell & add(std::unique_ptr<Widget> child)*



**brief** Add a child widget to the current cell.

**child** Widget to adopt; ownership transfers to the table.

**return** Configurable Cell describing the child's placement.

### row

*public Table & row()*



**brief** Advance to the next grid row.

**return** Reference to this table for chaining.

### set_padding

*public void set_padding(float padding)*



**brief** Set table padding.

**padding** Padding in logical units; non-negative.

### set_spacing

*public void set_spacing(float spacing)*



**brief** Set spacing between rows and columns.

**spacing** Gap in logical units; non-negative.

### minimum_size

*public Size minimum_size(Painter & , const Skin & )*



**brief** Report the smallest size fitting every cell.

**note** Parameters match Widget::minimum_size: an active painter and the skin providing style values.

**return** Minimum extent in logical units.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the preferred size from cell content.

**painter** Active painter used for measurement.

**skin** Skin forwarded to every cell widget.

**return** Preferred extent in logical units.

### layout

*public void layout(Painter & , const Skin & )*



**brief** Compute cell rectangles and position every child.

**note** Parameters match Widget::layout: an active painter and the skin providing style values.



