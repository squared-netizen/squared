# time &mdash; internals

Clock domains and a bounded deadline queue.

Programmer counterpart: [../programmer/time.md](../programmer/time.md)

- Public types: 6
- Translation units: 2

## Layout

```
time/
  include/squared/time/   one header per public type, plus time.hpp
  src/                        one translation unit per type with definitions
  Makefile
```

Types with no out-of-line definitions have no `.cpp`. Adding one means adding
its file to `SOURCES` in the module's `Makefile`.

## Notes

No module-specific deviations recorded yet.
