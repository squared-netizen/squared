# class LayoutWriter

*Defined at [gui/layout_writer.hpp:291](../../../../gui/layout_writer.hpp#L291)*



**brief** Writes a layout from C++, into storage the caller gives: what a C++ screen's `build()` does. The result is the same `Layout` a layout file compiles to, so one instantiator builds both.

 ```cpp root.add<sq::gui::CheckBox>( "Sound" ).bind( names::settings_sound ); root.row(); root.add<sq::gui::Button>( "Back" ).tap( sq::gui::Navigate::back ); ```

 Nodes go in document order, so a container takes nodes until a node is added to one outside it; adding to it after that is a mistake (the node is not written). Allocates nothing: words are copied into the text storage.



## Functions

### LayoutWriter

*public void LayoutWriter(std::span<LayoutNode> nodes, std::span<char> text, std::span<float> numbers, std::string_view name, LayoutKind kind)*

### LayoutWriter

*public void LayoutWriter(const LayoutWriter & )*

*Defined at [gui/layout_writer.hpp:296](../../../../gui/layout_writer.hpp#L296)*

### operator=

*public LayoutWriter & operator=(const LayoutWriter & )*

*Defined at [gui/layout_writer.hpp:297](../../../../gui/layout_writer.hpp#L297)*

### LayoutWriter

*public void LayoutWriter(LayoutWriter && )*

*Defined at [gui/layout_writer.hpp:298](../../../../gui/layout_writer.hpp#L298)*

### operator=

*public LayoutWriter & operator=(LayoutWriter && )*

*Defined at [gui/layout_writer.hpp:299](../../../../gui/layout_writer.hpp#L299)*

### ~LayoutWriter

*public void ~LayoutWriter()*

*Defined at [gui/layout_writer.hpp:300](../../../../gui/layout_writer.hpp#L300)*

### add

*public NodeWriter add(std::string_view text)*

*Defined at [gui/layout_writer.hpp:304](../../../../gui/layout_writer.hpp#L304)*



**brief** Add a node to the layout's own table. 

### add_custom

*public NodeWriter add_custom(std::string_view name)*



**brief** Add a custom node: a type the app registers by `name`. 

### add_list

*public NodeWriter add_list(const K & rows)*

*Defined at [gui/layout_writer.hpp:326](../../../../gui/layout_writer.hpp#L326)*



**brief** Add a list showing `rows`, a model's list (names.json's `"inventory": { "list": "item" }`): then add its row to it, one node, built once for each row on screen.

 ```cpp auto row = root.add_list( names::inventory ).grow().add<sq::gui::Table>(); row.add<sq::gui::Label>().bind( names::item_name );   // the row's item's name row.add<sq::gui::Button>( "Use" ).tap( names::item_use ); ```

 In a row, keys that take a uid get the row's. A list in a row is refused.

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

*Defined at [gui/layout_writer.hpp:335](../../../../gui/layout_writer.hpp#L335)*



**brief** How the layout shows when it is a pane (`Layout::pane`). 

### complete

*public bool complete()*

*Defined at [gui/layout_writer.hpp:341](../../../../gui/layout_writer.hpp#L341)*



**brief** Everything asked for was written: nothing ran out of room or went where it cannot. 



