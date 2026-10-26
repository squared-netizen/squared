# class LayoutWriter

*Defined at [gui/layout_writer.hpp:255](../../../../gui/layout_writer.hpp#L255)*



**brief** Writes a layout from C++, into storage the caller gives: what a C++ screen's `build()` does. The result is the same `Layout` a layout file compiles to, so one instantiator builds both.

 ```cpp root.add<sq::gui::CheckBox>( "Sound" ).bind( names::settings_sound ); root.row(); root.add<sq::gui::Button>( "Back" ).tap( sq::gui::Navigate::back ); ```

 Nodes go in document order, so a container takes nodes until a node is added to one outside it; adding to it after that is a mistake (the node is not written). Allocates nothing: words are copied into the text storage.



## Functions

### LayoutWriter

*public void LayoutWriter(std::span<LayoutNode> nodes, std::span<char> text, std::span<float> numbers, std::string_view name, LayoutKind kind)*

### LayoutWriter

*public void LayoutWriter(const LayoutWriter & )*

*Defined at [gui/layout_writer.hpp:260](../../../../gui/layout_writer.hpp#L260)*

### operator=

*public LayoutWriter & operator=(const LayoutWriter & )*

*Defined at [gui/layout_writer.hpp:261](../../../../gui/layout_writer.hpp#L261)*

### LayoutWriter

*public void LayoutWriter(LayoutWriter && )*

*Defined at [gui/layout_writer.hpp:262](../../../../gui/layout_writer.hpp#L262)*

### operator=

*public LayoutWriter & operator=(LayoutWriter && )*

*Defined at [gui/layout_writer.hpp:263](../../../../gui/layout_writer.hpp#L263)*

### ~LayoutWriter

*public void ~LayoutWriter()*

*Defined at [gui/layout_writer.hpp:264](../../../../gui/layout_writer.hpp#L264)*

### add

*public NodeWriter add(std::string_view text)*

*Defined at [gui/layout_writer.hpp:268](../../../../gui/layout_writer.hpp#L268)*



**brief** Add a node to the layout's own table. 

### add_custom

*public NodeWriter add_custom(std::string_view name)*



**brief** Add a custom node: a type the app registers by `name`. 

### complete

*public bool complete()*

*Defined at [gui/layout_writer.hpp:283](../../../../gui/layout_writer.hpp#L283)*



**brief** Everything asked for was written: nothing ran out of room or went where it cannot. 

### row

*public LayoutWriter & row()*



**brief** The next node added to the layout's own table starts a new row. 

### layout

*public Layout layout()*



**brief** What was written, as a layout. Points into the storage. 

### clear

*public void clear()*



**brief** Start again, with the same storage. 



