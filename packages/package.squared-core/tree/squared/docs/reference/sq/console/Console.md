# class Console

*Defined at [console/console.cppm:44](../../../../console/console.cppm#L44)*

## Records

Queued



## Functions

### Console

*public void Console(const Console & )*

*Defined at [console/console.cppm:50](../../../../console/console.cppm#L50)*

### operator=

*public Console & operator=(const Console & )*

*Defined at [console/console.cppm:51](../../../../console/console.cppm#L51)*

### Console

*public void Console(Console && )*

*Defined at [console/console.cppm:52](../../../../console/console.cppm#L52)*

### Console

*public void Console()*



**brief** An empty, hidden console with its built-ins: `help`, `clear`, `close`, `dump`. 

### operator=

*public Console & operator=(Console && )*

*Defined at [console/console.cppm:53](../../../../console/console.cppm#L53)*

### add

*public Commands::AddResult add(const char * name, const char * help, Handler handler, void * context)*

*Defined at [console/console.cppm:56](../../../../console/console.cppm#L56)*

### add_group

*public Groups::AddResult add_group(const char * group, std::initializer_list<const char *> lines)*



**brief** Add each of `lines` to `group`; the first refusal, or `added`. 

### add_to_group

*public Groups::AddResult add_to_group(const char * group, const char * line)*

*Defined at [console/console.cppm:66](../../../../console/console.cppm#L66)*

### dumping

*public bool dumping()*

*Defined at [console/console.cppm:82](../../../../console/console.cppm#L82)*

### set_mirror

*public void set_mirror(Mirror mirror, void * context)*

*Defined at [console/console.cppm:88](../../../../console/console.cppm#L88)*

### lines

*public Scrollback & lines()*

*Defined at [console/console.cppm:139](../../../../console/console.cppm#L139)*

### input

*public const InputLine & input()*

*Defined at [console/console.cppm:140](../../../../console/console.cppm#L140)*

### input

*public InputLine & input()*

*Defined at [console/console.cppm:141](../../../../console/console.cppm#L141)*

### commands

*public const Commands & commands()*

*Defined at [console/console.cppm:142](../../../../console/console.cppm#L142)*

### groups

*public const Groups & groups()*

*Defined at [console/console.cppm:143](../../../../console/console.cppm#L143)*

### update

*public void update()*



**brief** Run the next line `dump` queued, if any: once a frame, so a long group never holds a frame up, and the bridge sends between.

### print

*public void print(std::string_view text, Level level)*



**brief** Add text to the scrollback, one line per `**n** ` part. 

### printf

*public void printf(Level level, const char * format)*



**brief** Format with printf rules into one fixed buffer (format_bytes), then print. 

### run

*public void run(std::string_view line)*



**brief** Run a command line: echo it, split it, call its command.

 An unknown name, or too many words, prints an error line. The words are views into `line`, which must stay put until this returns.

### shown

*public bool shown()*

*Defined at [console/console.cppm:134](../../../../console/console.cppm#L134)*

### show

*public void show()*

*Defined at [console/console.cppm:135](../../../../console/console.cppm#L135)*

### hide

*public void hide()*

*Defined at [console/console.cppm:136](../../../../console/console.cppm#L136)*

### lines

*public const Scrollback & lines()*

*Defined at [console/console.cppm:138](../../../../console/console.cppm#L138)*

### submit

*public void submit()*



**brief** Run the input line (Enter): remember it in the history, run it, clear it. 

### complete

*public std::size_t complete()*



**brief** Complete the command name being typed (Tab).

**return** How many commands match. One: the name is filled in. More: the common part is filled in and the matches are printed.

### key

*public bool key(app::Key key, app::KeyModifiers modifiers)*



**brief** Edit the input with a key: arrows, Home, End, Backspace, Delete, Enter (submit), Tab (complete), Escape (clear the line).

**return** true when the key was the console's.

### queue_group

*public Queued queue_group(std::string_view group)*



**brief** Queue each line of `group` that is not queued already, for update() to run; `all` queues every group, in the order made.

### stop_dump

*public std::size_t stop_dump()*



**brief** Forget the queued lines; how many there were. 

### ~Console

*public void ~Console()*

*Defined at [console/console.cppm:49](../../../../console/console.cppm#L49)*

### text

*public void text(std::string_view utf8)*

*Defined at [console/console.cppm:127](../../../../console/console.cppm#L127)*



