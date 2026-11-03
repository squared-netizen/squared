# struct PtyError

*Defined at [term/pty_error.cppm:30](../../../../term/pty_error.cppm#L30)*



**brief** Why `Pty::spawn` failed: true when it did.

 Owns no memory: the text is fixed, `system_error` is the `errno` of the call that failed. A program that cannot be run is not one of these: the process was made, and it says so in the terminal and exits with 127.



## Members

public PtyErrorCode code

public int system_error



## Functions

### operator bool

*public bool operator bool()*

*Defined at [term/pty_error.cppm:35](../../../../term/pty_error.cppm#L35)*

### what

*public std::string_view what()*

*Defined at [term/pty_error.cppm:38](../../../../term/pty_error.cppm#L38)*

 The step that failed, in a few words. 

