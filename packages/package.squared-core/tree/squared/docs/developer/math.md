# math &mdash; internals

Small value types used across the framework.

Programmer counterpart: [../programmer/math.md](../programmer/math.md)

- Public types: 2
- Translation units: 1

## Layout

```
math/
  include/squared/math/   one header per public type, plus math.hpp
  src/                        one translation unit per type with definitions
  Makefile
```

Types with no out-of-line definitions have no `.cpp`. Adding one means adding
its file to `SOURCES` in the module's `Makefile`.

## Notes

No module-specific deviations recorded yet.
