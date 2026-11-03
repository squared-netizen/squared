# class Scrollback

*Defined at [term/src/scrollback.cppm:20](../../../../../term/src/scrollback.cppm#L20)*

 The lines that scrolled off the top of the main screen, in one ring of cells allocated when its size is set: each line with its trailing blanks trimmed, between a header and a footer cell that hold its length (so the ring is walked from either end), oldest out to make room. A line is never split across the ring's end: what is left there is skipped. 24 bytes a line and 12 a cell.

 `line( back )` walks back from the newest, one step a line: reading the lines a scrolled-back view shows costs as many steps as it is scrolled.



## Records

Line



## Functions

### Scrollback

*public void Scrollback()*

*Defined at [term/src/scrollback.cppm:30](../../../../../term/src/scrollback.cppm#L30)*

### capacity_bytes

*public std::size_t capacity_bytes()*

*Defined at [term/src/scrollback.cppm:42](../../../../../term/src/scrollback.cppm#L42)*

### clear

*public void clear()*

 Every line forgotten; the memory kept. 

### line

*public Line line(std::size_t back)*

 Line `back`, 0 the newest; `back` is below `size()`. 

### push

*public void push(std::span<const Cell> line, bool wrapped)*

 A line, its trailing blanks trimmed; the oldest lines go to make room. 

### reset

*public void reset(std::size_t bytes)*

 Room for `bytes` (rounded down to whole cells), empty. 0: keeps nothing. 

### size

*public std::size_t size()*

*Defined at [term/src/scrollback.cppm:41](../../../../../term/src/scrollback.cppm#L41)*

