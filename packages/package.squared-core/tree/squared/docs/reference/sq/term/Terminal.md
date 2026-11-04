# class Terminal

*Defined at [term/terminal.cppm:41](../../../../term/terminal.cppm#L41)*



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

*Defined at [term/terminal.cppm:48](../../../../term/terminal.cppm#L48)*

### Terminal

*public void Terminal(const Terminal & )*

*Defined at [term/terminal.cppm:46](../../../../term/terminal.cppm#L46)*

### Terminal

*public void Terminal(const TerminalConfig & config)*

### clear_dirty

*public void clear_dirty()*

*Defined at [term/terminal.cppm:78](../../../../term/terminal.cppm#L78)*

### clear_replies

*public void clear_replies()*

*Defined at [term/terminal.cppm:141](../../../../term/terminal.cppm#L141)*

### columns

*public std::uint16_t columns()*

*Defined at [term/terminal.cppm:64](../../../../term/terminal.cppm#L64)*

### copy_text

*public std::size_t copy_text(TextPoint from, TextPoint to, std::span<char> out)*

 The text from `from` to `to` (both included, either first), as UTF-8, into `out`; the bytes it takes, all of them, whatever `out` holds. Scrollback lines, then the screen in use's rows, held to the lines there are. A line that wrapped runs on into the next; any other ends with LF, its trailing blanks left out. A wide character's second cell gives nothing.

 `out` takes whole characters, in order, until one does not fit: call it with an empty `out` for the size, then with that much. No allocation; finding the first scrollback line walks back to it.

### cursor

*public Cursor cursor()*

*Defined at [term/terminal.cppm:80](../../../../term/terminal.cppm#L80)*

### feed

*public void feed(std::span<const std::byte> bytes)*

 What the program wrote, in any pieces: a sequence split between two calls is read whole. 

### feed

*public void feed(std::string_view text)*

*Defined at [term/terminal.cppm:54](../../../../term/terminal.cppm#L54)*

### first_line

*public std::uint64_t first_line()*

*Defined at [term/terminal.cppm:97](../../../../term/terminal.cppm#L97)*

 Its lines are numbered from the first it took: the scrollback's oldest kept line is `first_line()`, the screen's row 0 is `screen_line()`, row r is `screen_line() + r`. A line keeps its number as more scroll off (they move up the scrollback, then out).

### history_line

*public std::span<const Cell> history_line(std::size_t back)*

*Defined at [term/terminal.cppm:86](../../../../term/terminal.cppm#L86)*

 Line `back`, 0 the newest; `back` is below `history_size()`. 

### history_size

*public std::size_t history_size()*

*Defined at [term/terminal.cppm:84](../../../../term/terminal.cppm#L84)*

 Lines scrolled off the top of the main screen. 

### modes

*public TerminalModes modes()*

*Defined at [term/terminal.cppm:81](../../../../term/terminal.cppm#L81)*

### operator=

*public Terminal & operator=(Terminal && )*

*Defined at [term/terminal.cppm:49](../../../../term/terminal.cppm#L49)*

### operator=

*public Terminal & operator=(const Terminal & )*

*Defined at [term/terminal.cppm:47](../../../../term/terminal.cppm#L47)*

### replies

*public std::span<const std::byte> replies()*

*Defined at [term/terminal.cppm:137](../../../../term/terminal.cppm#L137)*

 Bytes for the program (replies to a device status report or a device attributes request), for the owner to write to it; then `clear_replies`. At most 256 wait; more are dropped.

### resize

*public void resize(std::uint16_t columns, std::uint16_t rows)*

 A new size (at least 1 x 1). The main screen keeps its top left; when it loses rows below the cursor, the rows above go to the scrollback first, so the cursor's line stays in view. Lines are not rewrapped. Every row is marked dirty.

### row

*public std::span<const Cell> row(std::uint16_t row)*

*Defined at [term/terminal.cppm:68](../../../../term/terminal.cppm#L68)*

 Row `row` of the screen in use (the alternate one while a program has it). 

### row_dirty

*public bool row_dirty(std::uint16_t row)*

*Defined at [term/terminal.cppm:74](../../../../term/terminal.cppm#L74)*

 Changed since `clear_dirty`. 

### row_wrapped

*public bool row_wrapped(std::uint16_t row)*

*Defined at [term/terminal.cppm:104](../../../../term/terminal.cppm#L104)*

 Row `row` of the screen in use went on into the next by autowrap. 

### rows

*public std::uint16_t rows()*

*Defined at [term/terminal.cppm:65](../../../../term/terminal.cppm#L65)*

### screen_line

*public std::uint64_t screen_line()*

*Defined at [term/terminal.cppm:101](../../../../term/terminal.cppm#L101)*

### take_events

*public TerminalEvents take_events()*

*Defined at [term/terminal.cppm:130](../../../../term/terminal.cppm#L130)*

 What happened since the last call: the bell, a new title. 

### title

*public std::string_view title()*

*Defined at [term/terminal.cppm:124](../../../../term/terminal.cppm#L124)*

 The title a program set (OSC 0 or 2), UTF-8, at most 128 bytes. 

### ~Terminal

*public void ~Terminal()*

*Defined at [term/terminal.cppm:50](../../../../term/terminal.cppm#L50)*

