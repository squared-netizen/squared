# class LayoutWriter

*Defined at [gui/layout_writer.cppm:285](../../../../gui/layout_writer.cppm#L285)*

## Functions

### LayoutWriter

*public void LayoutWriter(std::span<LayoutNode> nodes, std::span<char> text, std::span<float> numbers, std::string_view name, LayoutKind kind)*

### LayoutWriter

*public void LayoutWriter(const LayoutWriter & )*

*Defined at [gui/layout_writer.cppm:290](../../../../gui/layout_writer.cppm#L290)*

### operator=

*public LayoutWriter & operator=(const LayoutWriter & )*

*Defined at [gui/layout_writer.cppm:291](../../../../gui/layout_writer.cppm#L291)*

### LayoutWriter

*public void LayoutWriter(LayoutWriter && )*

*Defined at [gui/layout_writer.cppm:292](../../../../gui/layout_writer.cppm#L292)*

### operator=

*public LayoutWriter & operator=(LayoutWriter && )*

*Defined at [gui/layout_writer.cppm:293](../../../../gui/layout_writer.cppm#L293)*

### ~LayoutWriter

*public void ~LayoutWriter()*

*Defined at [gui/layout_writer.cppm:294](../../../../gui/layout_writer.cppm#L294)*

### add

*public NodeWriter add(std::string_view text)*

*Defined at [gui/layout_writer.cppm:298](../../../../gui/layout_writer.cppm#L298)*

### add_custom

*public NodeWriter add_custom(std::string_view name)*



**brief** Add a custom node: a type the app registers by `name`. 

### add_list

*public NodeWriter add_list(const K & rows)*

*Defined at [gui/layout_writer.cppm:320](../../../../gui/layout_writer.cppm#L320)*

### row

*public LayoutWriter & row()*



**brief** The next node added to the layout's own table starts a new row. 

### layout

*public Layout layout()*



**brief** What was written, as a layout. Points into the storage. 

### clear

*public void clear()*



**brief** Start again, with the same storage. 

### set_pane

*public void set_pane(PaneShape shape)*

*Defined at [gui/layout_writer.cppm:329](../../../../gui/layout_writer.cppm#L329)*

### complete

*public bool complete()*

*Defined at [gui/layout_writer.cppm:335](../../../../gui/layout_writer.cppm#L335)*



