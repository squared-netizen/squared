# class Console

*Defined at [console/console.hpp:43](../../../../console/console.hpp#L43)*



**brief** Text in, text out: the escape hatch for when the rest is broken.

 This is the Console's state and behaviour, with no drawing: the scrollback, the input line, the command table. The view that draws it comes separately. Everything is inline and sized at compile time (about 68 KB by default, nearly all of it the scrollback), so hold it in something long-lived, never on the stack. Nothing it does allocates. One thread uses it.

 ```cpp console.add( "spawn", "spawn <monster> <count>",              []( sq::console::Console &c, const sq::console::Args &a, void * ) {                  c.printf( sq::console::Level::info, "spawned %lld %.*s",                            static_cast<long long>( a.integer( 2, 1 ) ),                            static_cast<int>( a.text( 1 ).size() ), a.text( 1 ).data() );              } ); console.run( "spawn goblin 3" ); ```



## Functions

### Console

*public void Console()*



**brief** An empty, hidden console with its built-ins: `help`, `clear`, `close`. 

### ~Console

*public void ~Console()*

*Defined at [console/console.hpp:48](../../../../console/console.hpp#L48)*

### Console

*public void Console(const Console & )*

*Defined at [console/console.hpp:49](../../../../console/console.hpp#L49)*

### operator=

*public Console & operator=(const Console & )*

*Defined at [console/console.hpp:50](../../../../console/console.hpp#L50)*

### Console

*public void Console(Console && )*

*Defined at [console/console.hpp:51](../../../../console/console.hpp#L51)*

### operator=

*public Console & operator=(Console && )*

*Defined at [console/console.hpp:52](../../../../console/console.hpp#L52)*

### add

*public Commands::AddResult add(const char * name, const char * help, Handler handler, void * context)*

*Defined at [console/console.hpp:55](../../../../console/console.hpp#L55)*



**brief** Register a command. See Commands::add for what can refuse. 

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

### set_mirror

*public void set_mirror(Mirror mirror, void * context)*

*Defined at [console/console.hpp:65](../../../../console/console.hpp#L65)*



**brief** Hand every printed line to `mirror` as well, the echo of a command and its output included; nullptr stops it. One at a time.

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

### text

*public void text(std::string_view utf8)*

*Defined at [console/console.hpp:104](../../../../console/console.hpp#L104)*



**brief** Typed text, from a keyboard or the soft keyboard. 

### shown

*public bool shown()*

*Defined at [console/console.hpp:111](../../../../console/console.hpp#L111)*



**brief** Whether it should be drawn. The Console only keeps the flag: whoever draws it reads it. Hidden at first; `close` and the close button hide it.

### show

*public void show()*

*Defined at [console/console.hpp:112](../../../../console/console.hpp#L112)*

### hide

*public void hide()*

*Defined at [console/console.hpp:113](../../../../console/console.hpp#L113)*

### lines

*public const Scrollback & lines()*

*Defined at [console/console.hpp:115](../../../../console/console.hpp#L115)*

### lines

*public Scrollback & lines()*

*Defined at [console/console.hpp:116](../../../../console/console.hpp#L116)*

### input

*public const InputLine & input()*

*Defined at [console/console.hpp:117](../../../../console/console.hpp#L117)*

### input

*public InputLine & input()*

*Defined at [console/console.hpp:118](../../../../console/console.hpp#L118)*

### commands

*public const Commands & commands()*

*Defined at [console/console.hpp:119](../../../../console/console.hpp#L119)*



