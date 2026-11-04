# class Scrollback

*Defined at [term/src/scrollback.cppm:25](../../../../../term/src/scrollback.cppm#L25)*

 The lines that scrolled off the top of the main screen, in one ring of cells allocated when its size is set: each line with its trailing blanks trimmed, between a header and a footer cell that hold its length (so the ring is walked from either end), oldest out to make room. A line is never split across the ring's end: what is left there is skipped. 24 bytes a line and 12 a cell.

 `line( back )` walks back from the newest, one step a line: reading the lines a scrolled-back view shows costs as many steps as it is scrolled. `visit` finds its first line the same way, then walks forward (each line's header holds its length), so a run of lines costs one walk, not one each.

 Every line pushed is counted (`pushed`), kept or not: the terminal numbers its lines by it, so a line's number stays with it as more scroll off.



## Records

Line



## Functions

### Scrollback

*public void Scrollback()*

*Defined at [term/src/scrollback.cppm:35](../../../../../term/src/scrollback.cppm#L35)*

### capacity_bytes

*public std::size_t capacity_bytes()*

*Defined at [term/src/scrollback.cppm:47](../../../../../term/src/scrollback.cppm#L47)*

### clear

*public void clear()*

 Every line forgotten; the memory kept. 

### line

*public Line line(std::size_t back)*

 Line `back`, 0 the newest; `back` is below `size()`. 

### push

*public void push(std::span<const Cell> line, bool wrapped)*

 A line, its trailing blanks trimmed; the oldest lines go to make room. 

### pushed

*public std::uint64_t pushed()*

*Defined at [term/src/scrollback.cppm:56](../../../../../term/src/scrollback.cppm#L56)*

 Lines pushed since it was made, the evicted and the cleared among them. 

### reset

*public void reset(std::size_t bytes)*

 Room for `bytes` (rounded down to whole cells), empty. 0: keeps nothing. 

### size

*public std::size_t size()*

*Defined at [term/src/scrollback.cppm:46](../../../../../term/src/scrollback.cppm#L46)*

### visit

*public void visit(std::size_t back, std::size_t newest_back, F visit)*

*Defined at [term/src/scrollback.cppm:66](../../../../../term/src/scrollback.cppm#L66)*

 Lines `back` to `newest_back` (newest_back <= back < size()), the oldest first, each given to `visit( const Line & )`. `back` steps to the first, one for each after. `visit` by value: it is called once a line, so it is never forwarded (a lambda capturing by reference is a pointer or two).

