# console &mdash; internals

The Console module (Sprint 4). Programmer counterpart: [../programmer/console.md](../programmer/console.md). Design: the app-model spec, "The Console: the escape hatch".

## Where it lives, and why there

`squared/console/`, a module of squared-core, built into `libsquared_console.a`. A project links it only when `CONSOLE` is `app` or `monitor` (`mk/pkg_squared_core.mk`); with `CONSOLE=0`, the default, the archive is built with the framework but never linked, and no `SQUARED_CONSOLE*` flag reaches the compiler, so the APK carries none of it.

It was planned as its own package, `package.squared-console` (decided 2026-10-15). It became a module instead (2026-10-16). `vendor_framework.sh`, and sqpg's packages, put squared-core's `squared/` in place whole, so a second package writing into `squared/console/` would be removed by the next vendoring. Overlaying packages, or a second include root that `make headers` and `make lint` would have to learn, is tooling for a second consumer that does not exist. As a module, the existing checks cover it unchanged. Priority 4 (extensibility) lost to 2 (one tree, one vendoring step); priority 1 was unaffected either way, since an unlinked archive costs nothing.

## Sizes

`console/src/limits.hpp` reads `SQUARED_CONSOLE_SCROLLBACK` (default 65536) and `SQUARED_CONSOLE_COMMANDS` (default 64), checks them with `static_assert`, and exposes them as `scrollback_bytes` and `command_slots`. The template passes both, from `CONSOLE_SCROLLBACK` and `CONSOLE_COMMANDS`, to the framework's build (in `CXXFLAGS_EXTRA`) and to the application's (in `SQ_CPPFLAGS`) alike, and includes them in the framework's flags stamp, so archive and application cannot disagree about a size.

## Scrollback

**What it is:** a ring of variable-length line records in storage the owner provides. It owns no memory and never allocates.

**Record:** a 4-byte header (length as 16 bits, level, a zero byte), the text, padding to a multiple of 4. A line costs `4 + length`, rounded up to 4.

**Records never wrap around the end of the storage.** A record that does not fit above the tail ends the upper records there (`end_`), and the next one starts at 0, dropping the oldest until there is room below `head_`. So every line is contiguous, and handed out as a `string_view` into the storage, with no copy and no reassembly. The cost is the room above `end_`, at most one record's worth, unused until the head passes it.

| State | Records | Free |
|---|---|---|
| not wrapped | `head_ .. tail_` | `tail_ .. capacity` |
| wrapped | `head_ .. end_`, then `0 .. tail_` | `tail_ .. head_` |

| Operation | Time | Space |
|---|---|---|
| `push` of one line | O(lines dropped) plus the copy of its text | none |
| walking the lines | O(lines) | none |
| `last(n)` | O(size − n): it walks from the oldest | none |
| `clear` | O(1) | none |

`last(n)` walks forward because records carry no back links: a back link would cost 4 bytes per line, about 6% of a typical line, for a command (`copy 20`) a person types. Priority 1 over 3.

**Memory.** `sizeof(Scrollback)` is 56 on 64-bit (a span, two counters, four 32-bit offsets, a flag), checked with `static_assert( <= 64 )`. The storage is the Console's own array of `scrollback_bytes`: 64 KB by default, about 1,000 lines of 60 characters.

**Standard library.** `std::span` for the storage; `std::string_view` for lines. A `std::deque<std::string>` would allocate a node per line and a buffer per long line, and its size would depend on what was logged; the ring's is fixed at compile time.

**Tested** (`console/test/scrollback_test.cpp`, `make -C console test`): against a plain model (a deque with the same byte budget) over 20,000 random pushes into five storage sizes from 64 to 4096 bytes: the ring always holds a suffix of the model's lines, with the right levels, and every push is accounted for as held or dropped. Exact-fit records are all kept, before and after a wrap; an over-long line is cut; pushing 2,000 lines and walking them allocates nothing (the frame counter). Mutations that break the wrap, the end marker, or the exact-fit comparison each fail the test.
