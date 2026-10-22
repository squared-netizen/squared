# class LayoutWriter

*Defined at [gui/layout_writer.hpp:244](../../../../gui/layout_writer.hpp#L244)*



**brief** Writes a layout from C++, into storage the caller gives: what a C++ screen's `build()` does. The result is the same `Layout` a layout file compiles to, so one instantiator builds both.

 ```cpp root.add<sq::gui::CheckBox>( "Sound" ).bind( names::settings_sound ); root.row(); root.add<sq::gui::Button>( "Back" ).tap( sq::gui::Navigate::back ); ```

 Nodes go in document order, so a container takes nodes until a node is added to one outside it; adding to it after that is a mistake (the node is not written). Allocates nothing: words are copied into the text storage.



## Functions

### LayoutWriter

*public void LayoutWriter(std::span<LayoutNode> nodes, std::span<char> text, std::span<float> numbers, std::string_view name, LayoutKind kind)*

### LayoutWriter

*public void LayoutWriter(const LayoutWriter & )*

*Defined at [gui/layout_writer.hpp:249](../../../../gui/layout_writer.hpp#L249)*

### operator=

*public LayoutWriter & operator=(const LayoutWriter & )*

*Defined at [gui/layout_writer.hpp:250](../../../../gui/layout_writer.hpp#L250)*

### LayoutWriter

*public void LayoutWriter(LayoutWriter && )*

*Defined at [gui/layout_writer.hpp:251](../../../../gui/layout_writer.hpp#L251)*

### operator=

*public LayoutWriter & operator=(LayoutWriter && )*

*Defined at [gui/layout_writer.hpp:252](../../../../gui/layout_writer.hpp#L252)*

### ~LayoutWriter

*public void ~LayoutWriter()*

*Defined at [gui/layout_writer.hpp:253](../../../../gui/layout_writer.hpp#L253)*

### add

*public NodeWriter add(std::string_view text)*

*Defined at [gui/layout_writer.hpp:257](../../../../gui/layout_writer.hpp#L257)*



**brief** Add a node to the layout's own table. 

### add_custom

*public NodeWriter add_custom(std::string_view name)*



**brief** Add a custom node: a type the app registers by `name`. 

### complete

*public bool complete()*

*Defined at [gui/layout_writer.hpp:272](../../../../gui/layout_writer.hpp#L272)*



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



