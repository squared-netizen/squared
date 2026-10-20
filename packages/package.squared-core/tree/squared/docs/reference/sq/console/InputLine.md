# class InputLine

*Defined at [console/input_line.hpp:20](../../../../console/input_line.hpp#L20)*



**brief** The line being typed: text, a cursor, and the history of earlier lines.

 Text is UTF-8; the cursor moves and deletes whole characters. Up to `input_bytes` (256) are held, and `history_bytes` (2 KB) of earlier lines, all inline: editing never allocates.



## Functions

### InputLine

*public void InputLine()*

### ~InputLine

*public void ~InputLine()*

*Defined at [console/input_line.hpp:24](../../../../console/input_line.hpp#L24)*

### InputLine

*public void InputLine(const InputLine & )*

*Defined at [console/input_line.hpp:26](../../../../console/input_line.hpp#L26)*

 The history views the line's own storage, so the line cannot move.

### operator=

*public InputLine & operator=(const InputLine & )*

*Defined at [console/input_line.hpp:27](../../../../console/input_line.hpp#L27)*

### InputLine

*public void InputLine(InputLine && )*

*Defined at [console/input_line.hpp:28](../../../../console/input_line.hpp#L28)*

### insert

*public void insert(int utf8)*



**brief** Insert at the cursor. Control characters are dropped; what does not fit is cut at a character boundary. 

### erase_before

*public void erase_before()*



**brief** Delete the character before the cursor (Backspace). 

### erase_after

*public void erase_after()*



**brief** Delete the character after the cursor (Delete). 

### left

*public void left()*

### operator=

*public InputLine & operator=(InputLine && )*

*Defined at [console/input_line.hpp:29](../../../../console/input_line.hpp#L29)*

### home

*public void home()*

*Defined at [console/input_line.hpp:39](../../../../console/input_line.hpp#L39)*

### end

*public void end()*

*Defined at [console/input_line.hpp:40](../../../../console/input_line.hpp#L40)*

### right

*public void right()*

### set

*public void set(int utf8)*



**brief** Replace the text, cursor at its end. 

### clear

*public void clear()*



**brief** Empty the line; the history is kept. 

### previous

*public void previous()*



**brief** Show the previous history line (Up); the text being typed is kept for next(). 

### next

*public void next()*



**brief** Show the next history line (Down), and at the end the text that was being typed. 

### remember

*public void remember()*



**brief** Add the current text to the history, unless empty or the same as the newest. 

### text

*public int text()*

*Defined at [console/input_line.hpp:53](../../../../console/input_line.hpp#L53)*

### cursor

*public int cursor()*

*Defined at [console/input_line.hpp:55](../../../../console/input_line.hpp#L55)*



**brief** Cursor position in bytes, always at a character boundary. 

### history

*public const Scrollback & history()*

*Defined at [console/input_line.hpp:56](../../../../console/input_line.hpp#L56)*



