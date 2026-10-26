# class NodeWriter

*Defined at [gui/layout_writer.hpp:144](../../../../gui/layout_writer.hpp#L144)*



**brief** One node being written: each call sets something about it and returns it, so calls chain. A node that could not be written (the storage is full, or it was added where it cannot go) takes the calls and keeps nothing; `LayoutWriter::complete` says so.



## Functions

### grow

*public NodeWriter & grow()*



**brief** Take the cell's spare room both ways, or one. 

### grow_x

*public NodeWriter & grow_x()*

### grow_y

*public NodeWriter & grow_y()*

### fill

*public NodeWriter & fill()*



**brief** Fill the cell both ways, or one. 

### fill_x

*public NodeWriter & fill_x()*

### fill_y

*public NodeWriter & fill_y()*

### pad

*public NodeWriter & pad(float space)*



**brief** Space around it in its cell. 

### align

*public NodeWriter & align(Alignment x, Alignment y)*

### bind

*public NodeWriter & bind(const K & key)*

*Defined at [gui/layout_writer.hpp:171](../../../../gui/layout_writer.hpp#L171)*



**brief** Show `key`'s `state` (a label, check box, text field, slider or progress bar). 

### span

*public NodeWriter & span(std::uint8_t columns)*



**brief** The columns its cell takes. 

### style

*public NodeWriter & style(std::string_view name)*



**brief** A style from the skin, in place of `default`. 

### vertical

*public NodeWriter & vertical()*



**brief** A separator that runs up and down. 

### range

*public NodeWriter & range(float minimum, float maximum)*



**brief** A slider's or progress bar's range; `minimum` below `maximum`. 

### step

*public NodeWriter & step(float step)*



**brief** A slider's step; 0: none. 

### tap

*public NodeWriter & tap(std::string_view line)*



**brief** A button's tap: a line as a layout file writes it (`do file.open "a.txt"`, `push settings`). 

### tap

*public NodeWriter & tap(Navigate how, std::string_view target)*



**brief** A button's tap that navigates; `target` names the screen or pane. 

### tap

*public NodeWriter & tap(Navigate how)*

*Defined at [gui/layout_writer.hpp:184](../../../../gui/layout_writer.hpp#L184)*



**brief** A navigation to the C++ screen `S`, by its name: `.tap<About>( Navigate::push )`. 

### tap

*public NodeWriter & tap(const K & key)*

*Defined at [gui/layout_writer.hpp:191](../../../../gui/layout_writer.hpp#L191)*



**brief** A button's tap that sends `do <key>`. 

### tap

*public NodeWriter & tap(const K & key, const V & value)*

*Defined at [gui/layout_writer.hpp:201](../../../../gui/layout_writer.hpp#L201)*



**brief** A button's tap that sends `do <key> <value>`, or `set` for a key without `do`. 

### add

*public NodeWriter add(std::string_view text)*

*Defined at [gui/layout_writer.hpp:324](../../../../gui/layout_writer.hpp#L324)*



**brief** Add a node inside this table, scroll (one only) or stack. 

### add_custom

*public NodeWriter add_custom(std::string_view name)*



**brief** Add a custom node inside this one: a type the app registers by `name`. 

### row

*public NodeWriter & row()*



**brief** In a table: the next node added to it starts a new row. 

### written

*public bool written()*



**brief** Whether this node was written. 



