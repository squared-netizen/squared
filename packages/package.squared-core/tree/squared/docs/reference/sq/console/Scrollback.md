# class Scrollback

*Defined at [console/scrollback.cppm:28](../../../../console/scrollback.cppm#L28)*



**brief** The console's history: a ring of text lines, each with a level.

 It owns no memory. It is a view over storage its owner provides (the Console holds `std::array<char, scrollback_bytes>`), so its size is fixed at compile time and nothing is allocated, ever. When a new line does not fit, the oldest lines are dropped until it does.

 ```cpp std::array<char, sq::console::scrollback_bytes> storage; sq::console::Scrollback lines( storage ); lines.push( sq::console::Level::error, "shader did not link" ); for( const auto line : lines ) { draw( line.text, line.level ); } ```

 Not thread-safe: one thread pushes and reads.



## Records

Iterator



## Functions

### Scrollback

*public void Scrollback(std::span<char> storage)*



**brief** Use `storage` for the lines.

**storage** At least 64 bytes; kept, not copied. Bytes past the last multiple of 4 are not used.

### begin

*public Iterator begin()*



**brief** The oldest line. 

### clear

*public void clear()*



**brief** Forget every line; the dropped count is kept. 

### dropped

*public std::uint64_t dropped()*

*Defined at [console/scrollback.cppm:57](../../../../console/scrollback.cppm#L57)*



**brief** Lines dropped to make room, since construction. 

### empty

*public bool empty()*

*Defined at [console/scrollback.cppm:54](../../../../console/scrollback.cppm#L54)*



**brief** True when no line is held. 

### end

*public Iterator end()*

*Defined at [console/scrollback.cppm:104](../../../../console/scrollback.cppm#L104)*



**brief** Past the newest line. 

### last

*public Iterator last(std::size_t count)*



**brief** The newest `count` lines, oldest of them first.

**return** An iterator to walk to end(); all lines if `count` is larger.

### longest_line

*public std::size_t longest_line()*



**brief** The longest line push() keeps whole, in bytes. 

### push

*public void push(Level level, std::string_view text)*



**brief** Append text, one line per `**n** `-separated part.

 A trailing newline adds no empty line. A line longer than the storage can hold, or than 65535 bytes, is cut to fit. Drops the oldest lines as needed. Never allocates.

### size

*public std::size_t size()*

*Defined at [console/scrollback.cppm:51](../../../../console/scrollback.cppm#L51)*



**brief** Lines held now. 

