# class Layout

*Defined at [gui/layout.hpp:137](../../../../gui/layout.hpp#L137)*



**brief** A screen or pane as data: its nodes, their words and numbers. A view: it owns nothing, so what it points at outlives it. A compiled layout points into the binary's read-only data; a C++ screen's points into scratch memory (`LayoutWriter`).

 The text is every word one after another, each ended by '\0', and starts with a '\0' of its own, so offset 0 is "none".



## Functions

### Layout

*public void Layout()*

*Defined at [gui/layout.hpp:142](../../../../gui/layout.hpp#L142)*

### Layout

*public void Layout(std::string_view name, LayoutKind kind, std::span<const LayoutNode> nodes, std::string_view text, std::span<const float> numbers)*

*Defined at [gui/layout.hpp:143](../../../../gui/layout.hpp#L143)*

### name

*public std::string_view name()*

*Defined at [gui/layout.hpp:153](../../../../gui/layout.hpp#L153)*



**brief** Its name: what `push` and a saved screen stack call it. 

### kind

*public LayoutKind kind()*

*Defined at [gui/layout.hpp:154](../../../../gui/layout.hpp#L154)*

### nodes

*public Nodes nodes()*

*Defined at [gui/layout.hpp:155](../../../../gui/layout.hpp#L155)*

### numbers

*public std::span<const float> numbers()*

*Defined at [gui/layout.hpp:156](../../../../gui/layout.hpp#L156)*

### text

*public std::string_view text()*

*Defined at [gui/layout.hpp:159](../../../../gui/layout.hpp#L159)*



**brief** All of its text, as stored. 

### text

*public std::string_view text(std::uint16_t offset)*

*Defined at [gui/layout.hpp:162](../../../../gui/layout.hpp#L162)*



**brief** The words at `offset`; empty for 0, or outside the text. 



