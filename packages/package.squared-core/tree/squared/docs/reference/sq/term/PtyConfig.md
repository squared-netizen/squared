# struct PtyConfig

*Defined at [term/pty_config.cppm:14](../../../../term/pty_config.cppm#L14)*



**brief** The program a `Pty` starts, and the terminal it starts in.

 Only the views are held: what they name must outlive the `spawn` call, not the `Pty`.



## Members

public std::string_view program

public span arguments

public span environment

public std::string_view directory

public std::uint16_t columns

public std::uint16_t rows



