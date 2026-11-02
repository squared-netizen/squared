# class Bridge

*Defined at [console/bridge.cppm:85](../../../../console/bridge.cppm#L85)*

## Members

public static const std::size_t output_bytes

public static const std::size_t command_bytes

public static const std::size_t line_bytes

public static const std::uint64_t unknown_size



## Functions

### Bridge

*public void Bridge()*

*Defined at [console/bridge.cppm:98](../../../../console/bridge.cppm#L98)*

### ~Bridge

*public void ~Bridge()*

*Defined at [console/bridge.cppm:99](../../../../console/bridge.cppm#L99)*

### Bridge

*public void Bridge(const Bridge & )*

*Defined at [console/bridge.cppm:100](../../../../console/bridge.cppm#L100)*

### operator=

*public Bridge & operator=(const Bridge & )*

*Defined at [console/bridge.cppm:101](../../../../console/bridge.cppm#L101)*

### Bridge

*public void Bridge(Bridge && )*

*Defined at [console/bridge.cppm:102](../../../../console/bridge.cppm#L102)*

### open

*public BridgeStatus open(const BridgeConfig & config, const BridgeHandlers & handlers)*



**brief** Listen, and announce it. The secret is copied. 

### close

*public void close()*



**brief** Close every socket, telling `watch` to stop watching each. 

### operator=

*public Bridge & operator=(Bridge && )*

*Defined at [console/bridge.cppm:103](../../../../console/bridge.cppm#L103)*

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

*Defined at [console/bridge.cppm:142](../../../../console/bridge.cppm#L142)*

### listening

*public bool listening()*

*Defined at [console/bridge.cppm:144](../../../../console/bridge.cppm#L144)*

### connected

*public bool connected()*

*Defined at [console/bridge.cppm:145](../../../../console/bridge.cppm#L145)*

### port

*public std::uint16_t port()*

*Defined at [console/bridge.cppm:146](../../../../console/bridge.cppm#L146)*

### dropped

*public std::uint64_t dropped()*

*Defined at [console/bridge.cppm:148](../../../../console/bridge.cppm#L148)*



