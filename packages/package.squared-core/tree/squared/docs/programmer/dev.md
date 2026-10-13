# dev &mdash; development aids

Not part of a release build. Linked only when a project asks for it.

Developer counterpart: [../developer/dev.md](../developer/dev.md)

Header: `squared/dev/dev_log.hpp`

## Why it exists

On a phone with no computer attached there is no `adb logcat`, and Android
shows a crashed app as "keeps stopping" with no reason. The log sink sends each
log line, and a backtrace if the app dies, to a UDP port on the same device.
You read it in a terminal on that device. It uses the loopback address only: no
network, no Wi-Fi, and nothing leaves the device.

## The shortest use

In the Android template, one flag does everything:

```sh
make DEV_LOG=1 apk && make install      # build with the sink
nc -klu 5555                            # in Termux: print what arrives
```

Open the app in another window. Log lines appear as they are written. If the
app crashes, the last lines are:

```
CRASH SIGSEGV (11) code 1 addr 0x0
  #0 pc 0x1ba7 libdemo.so
  #1 pc 0x2c25 libdemo.so
  ...
END
```

Each `pc` is an offset into the named library. Turn it into a function and
line with the unstripped library from your build:

```sh
addr2line -f -C -e build/libdemo.so 0x1ba7
```

A normal build (`make apk`) contains none of this and does not request the
`INTERNET` permission. The template adds that permission to a copy of the
manifest only for `DEV_LOG` builds.

## LogSinkOptions

```cpp
struct LogSinkOptions {
    std::uint16_t port = 5555;  // UDP port on 127.0.0.1
    bool catch_crashes = true;  // report fatal signals
};
```

## open_log_sink

```cpp
[[nodiscard]] LogSinkStatus open_log_sink(const LogSinkOptions& options = {}) noexcept;
```

Opens the socket and, unless `catch_crashes` is false, installs handlers for
SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL and SIGTRAP.

| Returns | Meaning |
|---|---|
| `Opened` | Ready. |
| `AlreadyOpen` | A sink is already open; nothing changed. |
| `SocketFailed` | The OS refused the socket. On Android this is almost always the missing `INTERNET` permission. |

Failure is a returned value; nothing is thrown.

## log_line

```cpp
void log_line(std::string_view line) noexcept;
```

Sends one line. Does nothing if no sink is open. Lines over 1000 bytes are
cut. It never blocks; if nothing is listening the line is dropped.

```cpp
sq::dev::log_line("loaded skin");
```

## close_log_sink

```cpp
void close_log_sink() noexcept;
```

Closes the socket and restores the signal handlers it replaced.

## Lua

Not bound. This is a native debugging aid.

## Notes

- After reporting, the crash handler restores the default action and re-raises
  the signal, so Android still produces its own report and the process still
  dies the way it would have.
- A crash that happens before `open_log_sink` runs is not reported. The
  Android template opens the sink when the library loads, to cover startup.
