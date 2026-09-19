# data &mdash; internals

Strict RFC 8259 JSON over yyjson, behind an owned value type.

Programmer counterpart: [../programmer/data.md](../programmer/data.md)

- Public types: 7
- Translation units: 2

## Layout

```
data/
  include/squared/data/   one header per public type, plus json.hpp
  src/json_value.cpp      JsonValue's own members - no yyjson
  src/json.cpp            parse_json, write_json, and the conversion helpers
  Makefile
```

## yyjson is now one translation unit's problem

Before the split, the whole module was one translation unit and every part of
it was compiled with `yyjson.h` in scope. After it, `json_value.cpp` &mdash;
`JsonValue`'s 23 member definitions &mdash; has no yyjson dependency at all.
Only `json.cpp` includes it, because only `parse_json`, `write_json` and the
anonymous-namespace converters that back them touch the C API.

That is the practical payoff of the split here: the backend is contained in one
file rather than being a property of the module, and swapping it means
rewriting `json.cpp` and nothing else.

The four conversion helpers (`make_error`, `make_parse_error`,
`convert_value`, `convert_for_writing`) stayed in `json.cpp`'s anonymous
namespace rather than moving to a `detail` header, because only that
translation unit uses them. The rule throughout this tree: a helper leaves the
anonymous namespace only when a second translation unit needs it.

## Build dependency

yyjson is vendored under `third_party/yyjson/` and compiled into
`libsquared_data.a` by `data/Makefile`. `./tools/vendor_third_party.sh`
unpacks it from the release tarball in `third_party/`.

It is vendored rather than taken from a system package because this tree is
built with the NDK toolchain for an APK: a package installed for the host
sysroot is the wrong target.

Two details in the rule worth keeping:

- `yyjson.c` is compiled with `$(CXX) -x c -std=c11`, not with `$(CC)`. There
  is one toolchain variable to set, and `CC` would otherwise default to the
  host `cc` and silently produce host objects in an NDK build.
- It is compiled with `-w`. squared's own warning set includes `-Wconversion`
  and `-Wsign-conversion`; a JSON parser under those emits enough noise to bury
  a real warning from squared, and third-party code is not ours to keep clean.

The build fails with a named message rather than a missing-header error when
`third_party/yyjson` is absent:

```
data: third_party/yyjson is missing.
data: run ./tools/vendor_third_party.sh to unpack it.
```

`third_party/lua-*.tar.gz` is present for the scripting layer and is
deliberately left packed: nothing builds it yet, and unpacking it would make it
look wired up.

## Error handling

This module already does what the framework rules ask for and the GUI does not:
`parse_json` and `write_json` are `noexcept` and return a result carrying a
`JsonError`. There is nothing here on the `-fno-exceptions` blocking list.

Accessors are the `*_if()` pointer form rather than a throwing `as<T>()`, so
reading a value of the wrong type is a null check rather than an exception.

## Memory

`JsonValue` holds a `std::variant` over `std::nullptr_t`, `bool`, `std::int64_t`,
`std::uint64_t`, `double`, `std::string`, `Array` and `Object`. `Object` is a
`std::map`, which is one heap node per key plus a `std::string` key &mdash; the
same pattern flagged for `gui::Skin` in [priority-audit.md](priority-audit.md),
and for the same reason. It matters less here: JSON documents are parsed,
consumed, and dropped, rather than being resident for the life of the process.

A parsed document is copied out of yyjson's arena into owned storage, so the
`yyjson_doc` is freed before `parse_json` returns and nothing in the public API
points into it.
