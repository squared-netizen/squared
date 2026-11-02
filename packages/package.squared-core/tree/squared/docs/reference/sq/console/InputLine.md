# class InputLine

*Defined at [console/input_line.cppm:17](../../../../console/input_line.cppm#L17)*

## Functions

### InputLine

*public void InputLine()*

### ~InputLine

*public void ~InputLine()*

*Defined at [console/input_line.cppm:21](../../../../console/input_line.cppm#L21)*

### InputLine

*public void InputLine(const InputLine & )*

*Defined at [console/input_line.cppm:23](../../../../console/input_line.cppm#L23)*

### operator=

*public InputLine & operator=(const InputLine & )*

*Defined at [console/input_line.cppm:24](../../../../console/input_line.cppm#L24)*

### InputLine

*public void InputLine(InputLine && )*

*Defined at [console/input_line.cppm:25](../../../../console/input_line.cppm#L25)*

### insert

*public void insert(std::string_view utf8)*



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

*Defined at [console/input_line.cppm:26](../../../../console/input_line.cppm#L26)*

### home

*public void home()*

*Defined at [console/input_line.cppm:36](../../../../console/input_line.cppm#L36)*

### end

*public void end()*

*Defined at [console/input_line.cppm:37](../../../../console/input_line.cppm#L37)*

### right

*public void right()*

### set

*public void set(std::string_view utf8)*



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

*public std::string_view text()*

*Defined at [console/input_line.cppm:50](../../../../console/input_line.cppm#L50)*

### cursor

*public std::size_t cursor()*

*Defined at [console/input_line.cppm:52](../../../../console/input_line.cppm#L52)*

### history

*public const Scrollback & history()*

*Defined at [console/input_line.cppm:53](../../../../console/input_line.cppm#L53)*



