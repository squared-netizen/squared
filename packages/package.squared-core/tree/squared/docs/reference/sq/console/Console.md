# class Console

*Defined at [console/console.cppm:44](../../../../console/console.cppm#L44)*



**brief** Text in, text out: the escape hatch for when the rest is broken.

 This is the Console's state and behaviour, with no drawing: the scrollback, the input line, the command table. The view that draws it comes separately. Everything is inline and sized at compile time (about 68 KB by default, nearly all of it the scrollback), so hold it in something long-lived, never on the stack. Nothing it does allocates. One thread uses it.

 ```cpp console.add( "spawn", "spawn <monster> <count>",              []( sq::console::Console &c, const sq::console::Args &a, void * ) {                  c.printf( sq::console::Level::info, "spawned %lld %.*s",                            static_cast<long long>( a.integer( 2, 1 ) ),                            static_cast<int>( a.text( 1 ).size() ), a.text( 1 ).data() );              } ); console.run( "spawn goblin 3" ); ```



## Records

Queued



## Functions

### Console

*public void Console()*



**brief** An empty, hidden console with its built-ins: `help`, `clear`, `close`, `dump`. 

### Console

*public void Console(Console && )*

*Defined at [console/console.cppm:52](../../../../console/console.cppm#L52)*

### Console

*public void Console(const Console & )*

*Defined at [console/console.cppm:50](../../../../console/console.cppm#L50)*

### add

*public Commands::AddResult add(const char * name, const char * help, Handler handler, void * context)*

*Defined at [console/console.cppm:56](../../../../console/console.cppm#L56)*



**brief** Register a command. See Commands::add for what can refuse. 

### add_group

*public Groups::AddResult add_group(const char * group, std::initializer_list<const char *> lines)*



**brief** Add each of `lines` to `group`; the first refusal, or `added`. 

### add_to_group

*public Groups::AddResult add_to_group(const char * group, const char * line)*

*Defined at [console/console.cppm:66](../../../../console/console.cppm#L66)*



**brief** Add `line` (a whole command line, `logcat 100`) to the group `group` for `dump`, making the group if it is new. See Groups::add.

### commands

*public const Commands & commands()*

*Defined at [console/console.cppm:142](../../../../console/console.cppm#L142)*

### complete

*public std::size_t complete()*



**brief** Complete the command name being typed (Tab).

**return** How many commands match. One: the name is filled in. More: the common part is filled in and the matches are printed.

### dumping

*public bool dumping()*

*Defined at [console/console.cppm:82](../../../../console/console.cppm#L82)*



**brief** Whether `dump` has lines still to run: the loop should not sleep long. 

### groups

*public const Groups & groups()*

*Defined at [console/console.cppm:143](../../../../console/console.cppm#L143)*

### hide

*public void hide()*

*Defined at [console/console.cppm:136](../../../../console/console.cppm#L136)*

### input

*public InputLine & input()*

*Defined at [console/console.cppm:141](../../../../console/console.cppm#L141)*

### input

*public const InputLine & input()*

*Defined at [console/console.cppm:140](../../../../console/console.cppm#L140)*

### key

*public bool key(app::Key key, app::KeyModifiers modifiers)*



**brief** Edit the input with a key: arrows, Home, End, Backspace, Delete, Enter (submit), Tab (complete), Escape (clear the line).

**return** true when the key was the console's.

### lines

*public Scrollback & lines()*

*Defined at [console/console.cppm:139](../../../../console/console.cppm#L139)*

### lines

*public const Scrollback & lines()*

*Defined at [console/console.cppm:138](../../../../console/console.cppm#L138)*

### operator=

*public Console & operator=(Console && )*

*Defined at [console/console.cppm:53](../../../../console/console.cppm#L53)*

### operator=

*public Console & operator=(const Console & )*

*Defined at [console/console.cppm:51](../../../../console/console.cppm#L51)*

### print

*public void print(std::string_view text, Level level)*



**brief** Add text to the scrollback, one line per `**n** ` part. 

### printf

*public void printf(Level level, const char * format)*



**brief** Format with printf rules into one fixed buffer (format_bytes), then print. 

### queue_group

*public Queued queue_group(std::string_view group)*



**brief** Queue each line of `group` that is not queued already, for update() to run; `all` queues every group, in the order made.

### run

*public void run(std::string_view line)*



**brief** Run a command line: echo it, split it, call its command.

 An unknown name, or too many words, prints an error line. The words are views into `line`, which must stay put until this returns.

### set_mirror

*public void set_mirror(Mirror mirror, void * context)*

*Defined at [console/console.cppm:88](../../../../console/console.cppm#L88)*



**brief** Hand every printed line to `mirror` as well, the echo of a command and its output included; nullptr stops it. One at a time.

### show

*public void show()*

*Defined at [console/console.cppm:135](../../../../console/console.cppm#L135)*

### shown

*public bool shown()*

*Defined at [console/console.cppm:134](../../../../console/console.cppm#L134)*



**brief** Whether it should be drawn. The Console only keeps the flag: whoever draws it reads it. Hidden at first; `close` and the close button hide it.

### stop_dump

*public std::size_t stop_dump()*



**brief** Forget the queued lines; how many there were. 

### submit

*public void submit()*



**brief** Run the input line (Enter): remember it in the history, run it, clear it. 

### text

*public void text(std::string_view utf8)*

*Defined at [console/console.cppm:127](../../../../console/console.cppm#L127)*



**brief** Typed text, from a keyboard or the soft keyboard. 

### update

*public void update()*



**brief** Run the next line `dump` queued, if any: once a frame, so a long group never holds a frame up, and the bridge sends between.

### ~Console

*public void ~Console()*

*Defined at [console/console.cppm:49](../../../../console/console.cppm#L49)*

