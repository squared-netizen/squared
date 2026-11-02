# struct CrashLine

*Defined at [dev/src/crash_line.cppm:13](../../../../dev/src/crash_line.cppm#L13)*



**brief** A line of a crash report under construction: plain character appends into a fixed buffer, so a signal handler can use it. No allocation, no stdio, no locale. Text past `capacity` is dropped.



## Members

public char[1001] text

public std::size_t length

public static const std::size_t capacity



## Functions

### add

*public void add(const char * s)*

*Defined at [dev/src/crash_line.cppm:20](../../../../dev/src/crash_line.cppm#L20)*

### add_decimal

*public void add_decimal(std::uint64_t v)*

*Defined at [dev/src/crash_line.cppm:26](../../../../dev/src/crash_line.cppm#L26)*

### add_hex

*public void add_hex(std::uint64_t v)*

*Defined at [dev/src/crash_line.cppm:38](../../../../dev/src/crash_line.cppm#L38)*

