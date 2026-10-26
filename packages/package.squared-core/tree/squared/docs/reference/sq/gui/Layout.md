# class Layout

*Defined at [gui/layout.hpp:165](../../../../gui/layout.hpp#L165)*



**brief** A screen or pane as data: its nodes, their words and numbers. A view: it owns nothing, so what it points at outlives it. A compiled layout points into the binary's read-only data; a C++ screen's points into scratch memory (`LayoutWriter`).

 The text is every word one after another, each ended by '\0', and starts with a '\0' of its own, so offset 0 is "none".



## Functions

### Layout

*public void Layout()*

*Defined at [gui/layout.hpp:170](../../../../gui/layout.hpp#L170)*

### Layout

*public void Layout(std::string_view name, LayoutKind kind, std::span<const LayoutNode> nodes, std::string_view text, std::span<const float> numbers, PaneShape pane)*

*Defined at [gui/layout.hpp:171](../../../../gui/layout.hpp#L171)*

### name

*public std::string_view name()*

*Defined at [gui/layout.hpp:183](../../../../gui/layout.hpp#L183)*



**brief** Its name: what `push` and a saved screen stack call it. 

### kind

*public LayoutKind kind()*

*Defined at [gui/layout.hpp:184](../../../../gui/layout.hpp#L184)*

### pane

*public PaneShape pane()*

*Defined at [gui/layout.hpp:186](../../../../gui/layout.hpp#L186)*



**brief** How it shows when it is a pane: its file's `place` and `modal`. 

### nodes

*public Nodes nodes()*

*Defined at [gui/layout.hpp:187](../../../../gui/layout.hpp#L187)*

### numbers

*public std::span<const float> numbers()*

*Defined at [gui/layout.hpp:188](../../../../gui/layout.hpp#L188)*

### text

*public std::string_view text()*

*Defined at [gui/layout.hpp:191](../../../../gui/layout.hpp#L191)*



**brief** All of its text, as stored. 

### text

*public std::string_view text(std::uint16_t offset)*

*Defined at [gui/layout.hpp:194](../../../../gui/layout.hpp#L194)*



**brief** The words at `offset`; empty for 0, or outside the text. 



