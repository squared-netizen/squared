# class Scrollback

*Defined at [console/scrollback.hpp:33](../../../../console/scrollback.hpp#L33)*



**brief** The console's history: a ring of text lines, each with a level.

 It owns no memory. It is a view over storage its owner provides (the Console holds `std::array<char, scrollback_bytes>`), so its size is fixed at compile time and nothing is allocated, ever. When a new line does not fit, the oldest lines are dropped until it does.

 ```cpp std::array<char, sq::console::scrollback_bytes> storage; sq::console::Scrollback lines( storage ); lines.push( sq::console::Level::error, "shader did not link" ); for( const auto line : lines ) { draw( line.text, line.level ); } ```

 Not thread-safe: one thread pushes and reads.



## Records

Iterator



## Functions

### Scrollback

*public void Scrollback(int storage)*



**brief** Use `storage` for the lines.

**storage** At least 64 bytes; kept, not copied. Bytes past the last multiple of 4 are not used.

### push

*public void push(Level level, int text)*



**brief** Append text, one line per `**n** `-separated part.

 A trailing newline adds no empty line. A line longer than the storage can hold, or than 65535 bytes, is cut to fit. Drops the oldest lines as needed. Never allocates.

### clear

*public void clear()*



**brief** Forget every line; the dropped count is kept. 

### size

*public int size()*

*Defined at [console/scrollback.hpp:56](../../../../console/scrollback.hpp#L56)*



**brief** Lines held now. 

### empty

*public bool empty()*

*Defined at [console/scrollback.hpp:59](../../../../console/scrollback.hpp#L59)*



**brief** True when no line is held. 

### dropped

*public int dropped()*

*Defined at [console/scrollback.hpp:62](../../../../console/scrollback.hpp#L62)*



**brief** Lines dropped to make room, since construction. 

### longest_line

*public int longest_line()*



**brief** The longest line push() keeps whole, in bytes. 

### begin

*public Iterator begin()*



**brief** The oldest line. 

### end

*public Iterator end()*

*Defined at [console/scrollback.hpp:109](../../../../console/scrollback.hpp#L109)*



**brief** Past the newest line. 

### last

*public Iterator last(int count)*



**brief** The newest `count` lines, oldest of them first.

**return** An iterator to walk to end(); all lines if `count` is larger.



