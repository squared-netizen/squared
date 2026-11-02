# class NodeWriter

*Defined at [gui/layout_writer.cppm:148](../../../../gui/layout_writer.cppm#L148)*

## Functions

### add_list

*public NodeWriter add_list(const K & rows)*

*Defined at [gui/layout_writer.cppm:232](../../../../gui/layout_writer.cppm#L232)*

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

*Defined at [gui/layout_writer.cppm:181](../../../../gui/layout_writer.cppm#L181)*

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

*public NodeWriter & tap(Navigate how, std::string_view target)*



**brief** A button's tap that navigates; `target` names the screen or pane. 

### tap

*public NodeWriter & tap(Navigate how)*

*Defined at [gui/layout_writer.cppm:195](../../../../gui/layout_writer.cppm#L195)*

### tap

*public NodeWriter & tap(const K & key)*

*Defined at [gui/layout_writer.cppm:205](../../../../gui/layout_writer.cppm#L205)*

### tap

*public NodeWriter & tap(const K & key, const V & value)*

*Defined at [gui/layout_writer.cppm:218](../../../../gui/layout_writer.cppm#L218)*

### tap

*public NodeWriter & tap(std::string_view line)*



**brief** A button's tap: a line as a layout file writes it (`do file.open "a.txt"`, `push settings`). 

### add

*public NodeWriter add(std::string_view text)*

*Defined at [gui/layout_writer.cppm:377](../../../../gui/layout_writer.cppm#L377)*

### add_custom

*public NodeWriter add_custom(std::string_view name)*



**brief** Add a custom node inside this one: a type the app registers by `name`. 

### row

*public NodeWriter & row()*



**brief** In a table: the next node added to it starts a new row. 

### written

*public bool written()*



**brief** Whether this node was written. 



