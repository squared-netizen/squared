# class Terminal

*Defined at [term/terminal.cppm:40](../../../../term/terminal.cppm#L40)*



**brief** A terminal: the bytes a program writes in, a screen of cells out.

 It reads ECMA-48 (control functions, control sequences, SGR colours and attributes, operating system commands) over UTF-8, with the DEC and xterm functions sqterm takes: the alternate screen, scroll regions, cursor keys and visibility, bracketed paste. Compatibility with other terminals comes last (decided 2026-11-02): what it does not implement it ignores.

 It owns no pty, draws nothing and reads no keys: its owner feeds it what it reads from the program, draws the rows marked dirty, and writes back the replies (a cursor position report) to the program. So it runs on any thread, and in a test with no program at all.

 Memory: the two screens (rows x columns x 12 bytes each), the scrollback (1 MB unless configured), allocated by the constructor and by `resize`; `feed` never allocates. 80 x 24 with the default scrollback: about 1.05 MB.



 sq::term::Terminal terminal{ { .columns = 80, .rows = 24 } };

 terminal.feed( "\x1b[1mbold\x1b[0m plain\r\n" );

 auto first = terminal.row( 0 ); // first[0].code == U'b', bold





## Functions

### Terminal

*public void Terminal(Terminal && )*

*Defined at [term/terminal.cppm:47](../../../../term/terminal.cppm#L47)*

### Terminal

*public void Terminal(const Terminal & )*

*Defined at [term/terminal.cppm:45](../../../../term/terminal.cppm#L45)*

### Terminal

*public void Terminal(const TerminalConfig & config)*

### clear_dirty

*public void clear_dirty()*

*Defined at [term/terminal.cppm:77](../../../../term/terminal.cppm#L77)*

### clear_replies

*public void clear_replies()*

*Defined at [term/terminal.cppm:108](../../../../term/terminal.cppm#L108)*

### columns

*public std::uint16_t columns()*

*Defined at [term/terminal.cppm:63](../../../../term/terminal.cppm#L63)*

### cursor

*public Cursor cursor()*

*Defined at [term/terminal.cppm:79](../../../../term/terminal.cppm#L79)*

### feed

*public void feed(std::span<const std::byte> bytes)*

 What the program wrote, in any pieces: a sequence split between two calls is read whole. 

### feed

*public void feed(std::string_view text)*

*Defined at [term/terminal.cppm:53](../../../../term/terminal.cppm#L53)*

### history_line

*public std::span<const Cell> history_line(std::size_t back)*

*Defined at [term/terminal.cppm:85](../../../../term/terminal.cppm#L85)*

 Line `back`, 0 the newest; `back` is below `history_size()`. 

### history_size

*public std::size_t history_size()*

*Defined at [term/terminal.cppm:83](../../../../term/terminal.cppm#L83)*

 Lines scrolled off the top of the main screen. 

### modes

*public TerminalModes modes()*

*Defined at [term/terminal.cppm:80](../../../../term/terminal.cppm#L80)*

### operator=

*public Terminal & operator=(Terminal && )*

*Defined at [term/terminal.cppm:48](../../../../term/terminal.cppm#L48)*

### operator=

*public Terminal & operator=(const Terminal & )*

*Defined at [term/terminal.cppm:46](../../../../term/terminal.cppm#L46)*

### replies

*public std::span<const std::byte> replies()*

*Defined at [term/terminal.cppm:104](../../../../term/terminal.cppm#L104)*

 Bytes for the program (replies to a device status report or a device attributes request), for the owner to write to it; then `clear_replies`. At most 256 wait; more are dropped.

### resize

*public void resize(std::uint16_t columns, std::uint16_t rows)*

 A new size (at least 1 x 1). The main screen keeps its top left; when it loses rows below the cursor, the rows above go to the scrollback first, so the cursor's line stays in view. Lines are not rewrapped. Every row is marked dirty.

### row

*public std::span<const Cell> row(std::uint16_t row)*

*Defined at [term/terminal.cppm:67](../../../../term/terminal.cppm#L67)*

 Row `row` of the screen in use (the alternate one while a program has it). 

### row_dirty

*public bool row_dirty(std::uint16_t row)*

*Defined at [term/terminal.cppm:73](../../../../term/terminal.cppm#L73)*

 Changed since `clear_dirty`. 

### rows

*public std::uint16_t rows()*

*Defined at [term/terminal.cppm:64](../../../../term/terminal.cppm#L64)*

### take_events

*public TerminalEvents take_events()*

*Defined at [term/terminal.cppm:97](../../../../term/terminal.cppm#L97)*

 What happened since the last call: the bell, a new title. 

### title

*public std::string_view title()*

*Defined at [term/terminal.cppm:91](../../../../term/terminal.cppm#L91)*

 The title a program set (OSC 0 or 2), UTF-8, at most 128 bytes. 

### ~Terminal

*public void ~Terminal()*

*Defined at [term/terminal.cppm:49](../../../../term/terminal.cppm#L49)*

