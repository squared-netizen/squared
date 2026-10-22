# class Bridge

*Defined at [console/bridge.hpp:89](../../../../console/bridge.hpp#L89)*



**brief** The Console, reachable from Termux on the same device: every line it prints goes out, and every line that comes in is run as a command.

 A TCP port on 127.0.0.1, so nothing off the device can reach it, and a secret, so another app on it cannot either without reading this app's APK, which holds the secret. Dev builds only (the Android template opens it in CONSOLE builds). One client at a time: a newer one that gives the secret takes over.

 It waits on events, never on a timer: open() asks the owner, through `watch`, to tell it when a socket is ready, and ready() is how the owner does (the template uses Android's looper). Nothing allocates. Output waits in a fixed 16 KB buffer; when the client reads slower than lines arrive, lines are dropped and counted, and the client is told how many once there is room.

 Files travel both ways, one at a time each way, in 4 KB chunks read and written as the socket is ready, so a large one neither fills memory nor stalls the frame: send_file() sends one (the `pull` command, file_commands.hpp), and a client's `push` is written into BridgeConfig::push_directory.

 ```cpp sq::console::Bridge bridge; bridge.open( { .port = 30123, .secret = secret }, { .watch = watch,              .command = run_command, .connected = replay, .context = this } ); console.set_mirror( []( void *b, sq::console::Level level, std::string_view line ) noexcept {     static_cast<sq::console::Bridge *>( b )->send( level, line ); }, &bridge ); ```

 Wire format: console/src/bridge_protocol.hpp. Not thread-safe: one thread.



## Members

public static const std::size_t output_bytes

public static const std::size_t command_bytes

public static const std::size_t line_bytes

public static const std::uint64_t unknown_size



## Functions

### Bridge

*public void Bridge()*

*Defined at [console/bridge.hpp:102](../../../../console/bridge.hpp#L102)*

### ~Bridge

*public void ~Bridge()*

*Defined at [console/bridge.hpp:103](../../../../console/bridge.hpp#L103)*

### Bridge

*public void Bridge(const Bridge & )*

*Defined at [console/bridge.hpp:104](../../../../console/bridge.hpp#L104)*

### operator=

*public Bridge & operator=(const Bridge & )*

*Defined at [console/bridge.hpp:105](../../../../console/bridge.hpp#L105)*

### Bridge

*public void Bridge(Bridge && )*

*Defined at [console/bridge.hpp:106](../../../../console/bridge.hpp#L106)*

### open

*public BridgeStatus open(const BridgeConfig & config, const BridgeHandlers & handlers)*



**brief** Listen, and announce it. The secret is copied. 

### close

*public void close()*



**brief** Close every socket, telling `watch` to stop watching each. 

### operator=

*public Bridge & operator=(Bridge && )*

*Defined at [console/bridge.hpp:107](../../../../console/bridge.hpp#L107)*

### ready

*public bool ready(int fd, bool readable, bool writable)*



**brief** A socket `watch` named is ready.

**return** false when that socket is closed now (its watch already stopped).

### send

*public void send(Level level, std::string_view line)*



**brief** Send a line to the client, if one is connected; else nothing. 

### send_file

*public bool send_file(int fd, std::string_view name, std::uint64_t size)*



**brief** Send the open file `fd`, `size` bytes (or unknown_size), to the client as `name` (a plain name, no '/'). The bridge reads it as the socket takes it, and closes it. Called by a command, the command's end mark follows the file's last byte.

**return** false, leaving `fd` open, when no client is connected, a file is already on its way, or `name` is not a plain name.

### set_busy

*public void set_busy(bool busy)*



**brief** Whether a command goes on after it returned: a `dump` queues its lines and runs them one a frame. While busy, every command's end mark waits, so a client (`sqconsole --run`) reads on until the work is done; set_busy(false) sends them, in order. The owner sets it after running a command and after each `Console::update()`: `set_busy( console.dumping() )`.

### transferring

*public bool transferring()*

*Defined at [console/bridge.hpp:146](../../../../console/bridge.hpp#L146)*



**brief** A file is being sent (send_file) or received (a push). 

### listening

*public bool listening()*

*Defined at [console/bridge.hpp:148](../../../../console/bridge.hpp#L148)*

### connected

*public bool connected()*

*Defined at [console/bridge.hpp:149](../../../../console/bridge.hpp#L149)*

### port

*public std::uint16_t port()*

*Defined at [console/bridge.hpp:150](../../../../console/bridge.hpp#L150)*

### dropped

*public std::uint64_t dropped()*

*Defined at [console/bridge.hpp:152](../../../../console/bridge.hpp#L152)*



**brief** Lines dropped because the client read too slowly, since open(). 



