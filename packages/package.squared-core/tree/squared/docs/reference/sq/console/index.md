# namespace console

## Namespaces

* [bridge_protocol](bridge_protocol/index.md)
* [detail](detail/index.md)


## Records

* [Args](Args.md)
* [BridgeConfig](BridgeConfig.md)
* [BridgeHandlers](BridgeHandlers.md)
* [Bridge](Bridge.md)
* [ClipboardWriter](ClipboardWriter.md)
* [Command](Command.md)
* [Commands](Commands.md)
* [Console](Console.md)
* [Frame](Frame.md)
* [GridSize](GridSize.md)
* [InputLine](InputLine.md)
* [Line](Line.md)
* [Quad](Quad.md)
* [Renderer](Renderer.md)
* [Scrollback](Scrollback.md)


## Functions

### add_copy_command

*Commands::AddResult add_copy_command(Console & console, const ClipboardWriter & writer)*



**brief** Add the `copy` command, which writes lines of the scrollback to `writer`, one per line, separated by newlines:

 | Typed | Copies | |---|---| | `copy` | every line | | `copy errors` | the error lines only | | `copy 20` | the last 20 lines |

 The command's own echo (`> copy`) is not copied. It prints how many lines and bytes it copied, or that the clipboard is not available. `writer` is kept by address: it must outlive the Console. Nothing is allocated.

### add_crash_command

*Commands::AddResult add_crash_command(Console & console, const char * previous_path)*



**brief** Add the `crash` command: the report kept at `previous_path`, again; `crash segv` and `crash abort` crash the process on purpose, to see a report made. `previous_path` is kept, not copied: it must outlive the Console.

### add_file_commands

*int add_file_commands(Console & console, const char * home, Bridge * bridge)*



**brief** Add the commands for the app's own files:

 | Command | Does | |---|---| | `ls [path]` | the entries of a directory: type, size, name | | `pull <path>` | send a file to Termux through `bridge` (`make console` saves it) | | `push` | how to send one the other way (it is `make console`'s: `push <file> [name]`) |

 A relative path starts at `home` (the Android template: the app's data directory, so `files/crash.txt`, `cache/`, `lib/`); an absolute one is used as it is; `apk` is the installed APK. Everything the app may read, and only that: these run as the app.

 `home` and `bridge` are kept, not copied; `bridge` may be null, and then `pull` says it needs one. Nothing allocates.



**return** How many did not fit in the table (0 when all did).

### add_gl_command

*Commands::AddResult add_gl_command(Console & console)*



**brief** Add the `gl` command: the driver's vendor, renderer, version and shading language, and the limits that decide what a program can draw (texture and renderbuffer size, viewport, vertex attributes, texture units), plus the extension count. Built with the gles backend, like the Renderer; it must run where the context is current, as commands typed into the Console do.

### add_log_command

*Commands::AddResult add_log_command(Console & console, const char * previous_path)*



**brief** Add the `log` command: `log previous` shows the end of the last run's log, kept at `previous_path` by keep_previous_log(). `previous_path` is kept, not copied: it must outlive the Console.

### add_system_commands

*int add_system_commands(Console & console)*



**brief** Add the built-in commands that read the running system, through Linux interfaces that work on Android too:

 | Command | Prints | |---|---| | `mem` | resident and peak memory (`/proc/self/status`), and what the system has available (`/proc/meminfo`) | | `heap` | the allocator's in-use, free and mapped bytes (`mallinfo`) | | `threads` | the process's thread count | | `fds` | the open file descriptors | | `device` | maker, model, Android version, API level and ABI (system properties), and the kernel (`uname`) | | `smaps` | memory by kind (this program, native heap, GPU driver, Java runtime, stacks, other libraries, files, anonymous): PSS, RSS and private dirty, from `/proc/self/smaps` | | `libs` | every shared library the process has mapped, with the address space it takes (`/proc/self/maps`) |

 Each reads into the stack and prints through the Console: nothing is allocated, nothing is kept.



**return** How many did not fit in the command table: 0 when all were added.

### for_each_line

*void for_each_line(int fd, int buffer, Line line)*

*Defined at [console/line_reader.hpp:20](../../../../console/line_reader.hpp#L20)*



**brief** Read `fd` to its end through `buffer`, calling `line( text )` for each line, without its newline; the last line counts even with no newline after it. A line longer than the buffer arrives in buffer-sized pieces. Nothing is allocated: the buffer is the caller's, usually a few KB on the stack.

### grid

*GridSize grid(const Frame & frame)*

*Defined at [console/layout.hpp:43](../../../../console/layout.hpp#L43)*



**brief** The character grid of `frame`; zero when the cell size is not positive. 

### install_crash_file

*bool install_crash_file(const char * path, const Console * console, int log_lines)*



**brief** Write a report to `path` when the process dies of a fatal signal (SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGTRAP): the crash trace (`sq::dev::write_crash_trace`), then the newest `log_lines` lines of `console`'s scrollback, so the report says what led up to it.

 ```text CRASH SIGSEGV (11) code 1 addr 0x0   #0 pc 0x4a5f0 libdemo.so   ... END log: the last 40 lines   show landing: ... ```

 The file is created at the crash, never before, so an existing report is left alone until then; read it first (show_crash_file). The signal is then handed on to whatever handled it before (the dev log's, in a dev build, then Android's own crash report), so nothing else is lost. The handler allocates nothing and takes no lock; it runs on an alternate stack on this thread, so a stack overflow here is reported too.

 `path` is copied (at most 511 bytes); `console` must stay alive until remove_crash_file() or the end of the process. One installation at a time.



**return** false if `path` is too long or a handler could not be installed.

### keep_previous_log

*bool keep_previous_log(const char * path, const char * previous_path)*



**brief** Move the log the last run saved at `path` to `previous_path`, at start, before this run saves its own over it.

**return** true if there was one.

### layout

*void layout(const Console & console, const Frame & frame, Emit emit)*

*Defined at [console/layout.hpp:131](../../../../console/layout.hpp#L131)*



**brief** Where every character of the console goes, with no drawing.

 Calls `emit( Quad )` for each rectangle, back to front: the background, the title row with its close button `[x]`, the newest lines that fit (wrapped, newest at the bottom, moved back by `frame.scroll` rows), then the input row with its prompt and cursor. A renderer batches the quads; a test collects them. Nothing is allocated; it walks the scrollback twice.

 Characters outside ASCII are drawn as `?`: the font has 128 glyphs.

 ```cpp sq::console::layout( console, frame, [&]( const sq::console::Quad &quad ) { batch.add( quad ); } ); ```

### operator==

*bool operator==(const Iterator & a, const Iterator & b)*

*Defined at [console/scrollback.hpp:86](../../../../console/scrollback.hpp#L86)*

### remove_crash_file

*void remove_crash_file()*

 Put back the handlers install_crash_file() replaced. Before the Console goes away.

### run_program

*bool run_program(Console & console, const char *const * argv, int max_lines)*



**brief** Run a program and print what it writes (stdout and stderr), a line at a time, at most `max_lines` of them; then how many more there were. Waits for it to end, so for programs that do (`logcat -d`, not `logcat`).

 `argv` is the program's path, then its arguments, then nullptr; nothing is searched for on a PATH. It runs as this process does, so it reads what the app may read (on Android, logcat shows only the app's own lines). fork and exec, a pipe, a 2 KB buffer on the stack: nothing is allocated here.

 ```cpp const char *const logcat[] = { "/system/bin/logcat", "-d", "-t", "200", nullptr }; sq::console::run_program( console, logcat, 200 ); ```



**return** false when it could not run (no such program, no process).

### save_log

*bool save_log(const Console & console, const char * path, int count)*



**brief** Write the newest `count` lines of `console`'s scrollback to `path`, one per line, so the next run can show how this one ended.

 Written to `path` with `.tmp` added, then renamed over `path`, so a reader never finds half a file and a crash while writing leaves the last good one. Allocates nothing: the lines go straight from the scrollback to the file. Call it when the app may be about to go (paused, stopped, quitting); it costs one small file write.

 ```cpp sq::console::save_log( console, "/data/user/0/<package>/files/log.txt", 200 ); ```



**return** false if the file could not be written (the old one is kept), or `path` is longer than 500 bytes.

### scroll_limit

*int scroll_limit(const Console & console, const Frame & frame)*

*Defined at [console/layout.hpp:74](../../../../console/layout.hpp#L74)*



**brief** The largest useful `frame.scroll`: how many wrapped rows of lines there are beyond those the frame shows. A view scrolled further shows the oldest rows all the same; a caller keeping its own scroll clamps to this, so scrolling back down takes effect at once. O(scrollback), allocates nothing.

### show_crash_file

*bool show_crash_file(Console & console, const char * path, const char * previous_path)*



**brief** Print the report an earlier run left at `path` into `console`, a line at a time as errors, then move it to `previous_path` so it is shown once.

**return** true if there was a report.

### show_log

*bool show_log(Console & console, const char * path, int title)*



**brief** Print the file at `path` into `console`, a line at a time, after a `title` line.

**return** false, printing nothing, if there is no such file.

### touches_close

*bool touches_close(const Frame & frame, int x, int y)*

*Defined at [console/layout.hpp:52](../../../../console/layout.hpp#L52)*



**brief** True when the point (pixels) is on the close button, the last three cells of the title row. 

## Enums

| enum class BridgeStatus |

--

| listening |
| already_open |
| bad_secret |
| refused |


*Defined at [console/bridge.hpp:30](../../../../console/bridge.hpp#L30)*



**brief** What open() did. 

| enum class Colour |

--

| text |
| warning |
| error |
| dim |
| background |
| cursor |
| title |


*Defined at [console/src/colour.hpp:10](../../../../console/src/colour.hpp#L10)*



**brief** What a console quad is drawn in: an index into the renderer's palette. 

| enum class Level |

--

| info |
| warning |
| error |


*Defined at [console/src/level.hpp:9](../../../../console/src/level.hpp#L9)*



**brief** How a console line is shown and filtered: `copy errors` keeps only `error`. 

