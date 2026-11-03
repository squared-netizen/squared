# class Pty

*Defined at [term/pty.cppm:38](../../../../term/pty.cppm#L38)*



**brief** A program running in a pseudo-terminal: what it writes is read here, and fed to a `Terminal`; keys are written here.

 POSIX, the same on Linux and Android: `posix_openpt`, a new session for the child with the pseudo-terminal as its controlling terminal, `execve`. The terminal starts with UTF-8 input (IUTF8) and without XON/XOFF flow control (Ctrl-S and Ctrl-Q reach the program), as Termux's.

 Reads never block (`read` returns 0 when nothing waits); the owner waits on `fd()` with poll, or `wait_readable`. A write waits for room, up to its timeout. Destroying it, or `hang_up`, ends the program: SIGHUP to its process group, then SIGKILL if it is still there, and the process reaped.

 16 bytes, and nothing allocated but by `spawn` (the program's arguments, freed before it returns).



 sq::term::Pty pty;

 if( const auto error = pty.spawn( { .program = "/system/bin/sh" } ) ) {

     log( error.what() );

 }

 pty.write( "ls\r" );

 std::array<std::byte, 4096> buffer;

 terminal.feed( { buffer.data(), pty.read( buffer ) } );





## Functions

### Pty

*public void Pty()*

*Defined at [term/pty.cppm:41](../../../../term/pty.cppm#L41)*

### Pty

*public void Pty(Pty && other)*

### Pty

*public void Pty(const Pty & )*

*Defined at [term/pty.cppm:42](../../../../term/pty.cppm#L42)*

### closed

*public bool closed()*

*Defined at [term/pty.cppm:77](../../../../term/pty.cppm#L77)*

 No program will write any more: every process with the terminal open has closed it. 

### fd

*public int fd()*

*Defined at [term/pty.cppm:83](../../../../term/pty.cppm#L83)*

 The pseudo-terminal, to wait on with poll; -1 when none. 

### hang_up

*public void hang_up()*

 Ends the program (SIGHUP, then SIGKILL) and closes the terminal. 

### operator=

*public Pty & operator=(Pty && other)*

### operator=

*public Pty & operator=(const Pty & )*

*Defined at [term/pty.cppm:43](../../../../term/pty.cppm#L43)*

### pid

*public int pid()*

*Defined at [term/pty.cppm:85](../../../../term/pty.cppm#L85)*

 The program's process (its process group too); 0 when none. 

### poll_exit

*public std::optional<ProcessExit> poll_exit()*

 How the program ended, once it has (the process is then reaped); nothing while it runs or when none was started.

### read

*public std::size_t read(std::span<std::byte> into)*

 What the program wrote, up to `into.size()` bytes; 0 when nothing waits (or it is closed). 

### resize

*public void resize(std::uint16_t columns, std::uint16_t rows)*

 The terminal's new size: the program gets SIGWINCH. 

### spawn

*public PtyError spawn(const PtyConfig & config)*

 Starts `config.program`; an error (true) when it could not. 

### wait_readable

*public bool wait_readable(int timeout_ms)*

 Waits up to `timeout_ms` for something to read (or for the end); true when there is. 

### write

*public std::size_t write(std::span<const std::byte> bytes, int timeout_ms)*

 Bytes for the program (keys, a paste). Waits for room up to `timeout_ms` in all; returns how many were written.

### write

*public std::size_t write(std::string_view text, int timeout_ms)*

*Defined at [term/pty.cppm:59](../../../../term/pty.cppm#L59)*

### ~Pty

*public void ~Pty()*

