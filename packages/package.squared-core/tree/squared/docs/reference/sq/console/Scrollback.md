# class Scrollback

*Defined at [console/scrollback.cppm:28](../../../../console/scrollback.cppm#L28)*

## Records

Iterator



## Functions

### Scrollback

*public void Scrollback(std::span<char> storage)*



**brief** Use `storage` for the lines.

**storage** At least 64 bytes; kept, not copied. Bytes past the last multiple of 4 are not used.

### push

*public void push(Level level, std::string_view text)*



**brief** Append text, one line per `**n** `-separated part.

 A trailing newline adds no empty line. A line longer than the storage can hold, or than 65535 bytes, is cut to fit. Drops the oldest lines as needed. Never allocates.

### clear

*public void clear()*



**brief** Forget every line; the dropped count is kept. 

### size

*public std::size_t size()*

*Defined at [console/scrollback.cppm:51](../../../../console/scrollback.cppm#L51)*

### empty

*public bool empty()*

*Defined at [console/scrollback.cppm:54](../../../../console/scrollback.cppm#L54)*

### dropped

*public std::uint64_t dropped()*

*Defined at [console/scrollback.cppm:57](../../../../console/scrollback.cppm#L57)*

### longest_line

*public std::size_t longest_line()*



**brief** The longest line push() keeps whole, in bytes. 

### begin

*public Iterator begin()*



**brief** The oldest line. 

### end

*public Iterator end()*

*Defined at [console/scrollback.cppm:104](../../../../console/scrollback.cppm#L104)*

### last

*public Iterator last(std::size_t count)*



**brief** The newest `count` lines, oldest of them first.

**return** An iterator to walk to end(); all lines if `count` is larger.



