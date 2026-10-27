# class Layout

*Defined at [gui/layout.hpp:216](../../../../gui/layout.hpp#L216)*



**brief** A screen or pane as data: its nodes, their words and numbers. A view: it owns nothing, so what it points at outlives it. A compiled layout points into the binary's read-only data; a C++ screen's points into scratch memory (`LayoutWriter`).

 The text is every word one after another, each ended by '\0', and starts with a '\0' of its own, so offset 0 is "none".



## Functions

### Layout

*public void Layout()*

*Defined at [gui/layout.hpp:221](../../../../gui/layout.hpp#L221)*

### Layout

*public void Layout(std::string_view name, LayoutKind kind, std::span<const LayoutNode> nodes, std::string_view text, std::span<const float> numbers, PaneShape pane)*

*Defined at [gui/layout.hpp:222](../../../../gui/layout.hpp#L222)*

### name

*public std::string_view name()*

*Defined at [gui/layout.hpp:234](../../../../gui/layout.hpp#L234)*



**brief** Its name: what `push` and a saved screen stack call it. 

### kind

*public LayoutKind kind()*

*Defined at [gui/layout.hpp:235](../../../../gui/layout.hpp#L235)*

### pane

*public PaneShape pane()*

*Defined at [gui/layout.hpp:237](../../../../gui/layout.hpp#L237)*



**brief** How it shows when it is a pane: its file's `place` and `modal`. 

### nodes

*public Nodes nodes()*

*Defined at [gui/layout.hpp:238](../../../../gui/layout.hpp#L238)*

### numbers

*public std::span<const float> numbers()*

*Defined at [gui/layout.hpp:239](../../../../gui/layout.hpp#L239)*

### text

*public std::string_view text()*

*Defined at [gui/layout.hpp:242](../../../../gui/layout.hpp#L242)*



**brief** All of its text, as stored. 

### text

*public std::string_view text(std::uint16_t offset)*

*Defined at [gui/layout.hpp:245](../../../../gui/layout.hpp#L245)*



**brief** The words at `offset`; empty for 0, or outside the text. 



