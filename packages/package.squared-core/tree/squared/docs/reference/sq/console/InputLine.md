# class InputLine

*Defined at [console/input_line.cppm:17](../../../../console/input_line.cppm#L17)*



**brief** The line being typed: text, a cursor, and the history of earlier lines.

 Text is UTF-8; the cursor moves and deletes whole characters. Up to `input_bytes` (256) are held, and `history_bytes` (2 KB) of earlier lines, all inline: editing never allocates.



## Functions

### InputLine

*public void InputLine()*

### InputLine

*public void InputLine(InputLine && )*

*Defined at [console/input_line.cppm:25](../../../../console/input_line.cppm#L25)*

### InputLine

*public void InputLine(const InputLine & )*

*Defined at [console/input_line.cppm:23](../../../../console/input_line.cppm#L23)*

 The history views the line's own storage, so the line cannot move.

### clear

*public void clear()*



**brief** Empty the line; the history is kept. 

### cursor

*public std::size_t cursor()*

*Defined at [console/input_line.cppm:52](../../../../console/input_line.cppm#L52)*



**brief** Cursor position in bytes, always at a character boundary. 

### end

*public void end()*

*Defined at [console/input_line.cppm:37](../../../../console/input_line.cppm#L37)*

### erase_after

*public void erase_after()*



**brief** Delete the character after the cursor (Delete). 

### erase_before

*public void erase_before()*



**brief** Delete the character before the cursor (Backspace). 

### history

*public const Scrollback & history()*

*Defined at [console/input_line.cppm:53](../../../../console/input_line.cppm#L53)*

### home

*public void home()*

*Defined at [console/input_line.cppm:36](../../../../console/input_line.cppm#L36)*

### insert

*public void insert(std::string_view utf8)*



**brief** Insert at the cursor. Control characters are dropped; what does not fit is cut at a character boundary. 

### left

*public void left()*

### next

*public void next()*



**brief** Show the next history line (Down), and at the end the text that was being typed. 

### operator=

*public InputLine & operator=(InputLine && )*

*Defined at [console/input_line.cppm:26](../../../../console/input_line.cppm#L26)*

### operator=

*public InputLine & operator=(const InputLine & )*

*Defined at [console/input_line.cppm:24](../../../../console/input_line.cppm#L24)*

### previous

*public void previous()*



**brief** Show the previous history line (Up); the text being typed is kept for next(). 

### remember

*public void remember()*



**brief** Add the current text to the history, unless empty or the same as the newest. 

### right

*public void right()*

### set

*public void set(std::string_view utf8)*



**brief** Replace the text, cursor at its end. 

### text

*public std::string_view text()*

*Defined at [console/input_line.cppm:50](../../../../console/input_line.cppm#L50)*

### ~InputLine

*public void ~InputLine()*

*Defined at [console/input_line.cppm:21](../../../../console/input_line.cppm#L21)*

