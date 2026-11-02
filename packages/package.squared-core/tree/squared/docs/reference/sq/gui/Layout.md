# class Layout

*Defined at [gui/layout.cppm:210](../../../../gui/layout.cppm#L210)*



**brief** A screen or pane as data: its nodes, their words and numbers. A view: it owns nothing, so what it points at outlives it. A compiled layout points into the binary's read-only data; a C++ screen's points into scratch memory (`LayoutWriter`).

 The text is every word one after another, each ended by '\0', and starts with a '\0' of its own, so offset 0 is "none".



## Functions

### Layout

*public void Layout()*

*Defined at [gui/layout.cppm:215](../../../../gui/layout.cppm#L215)*

### Layout

*public void Layout(std::string_view name, LayoutKind kind, std::span<const LayoutNode> nodes, std::string_view text, std::span<const float> numbers, PaneShape pane)*

*Defined at [gui/layout.cppm:216](../../../../gui/layout.cppm#L216)*

### kind

*public LayoutKind kind()*

*Defined at [gui/layout.cppm:229](../../../../gui/layout.cppm#L229)*

### name

*public std::string_view name()*

*Defined at [gui/layout.cppm:228](../../../../gui/layout.cppm#L228)*



**brief** Its name: what `push` and a saved screen stack call it. 

### nodes

*public Nodes nodes()*

*Defined at [gui/layout.cppm:232](../../../../gui/layout.cppm#L232)*

### numbers

*public std::span<const float> numbers()*

*Defined at [gui/layout.cppm:233](../../../../gui/layout.cppm#L233)*

### pane

*public PaneShape pane()*

*Defined at [gui/layout.cppm:231](../../../../gui/layout.cppm#L231)*



**brief** How it shows when it is a pane: its file's `place` and `modal`. 

### text

*public std::string_view text()*

*Defined at [gui/layout.cppm:236](../../../../gui/layout.cppm#L236)*



**brief** All of its text, as stored. 

### text

*public std::string_view text(std::uint16_t offset)*

*Defined at [gui/layout.cppm:239](../../../../gui/layout.cppm#L239)*



**brief** The words at `offset`; empty for 0, or outside the text. 

