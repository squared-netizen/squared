# app &mdash; internals

The platform-neutral boundary: eight types, no translation units, no
dependencies beyond `sq::graphics::Context` (forward declared).

Programmer counterpart: [../programmer/app.md](../programmer/app.md)

- Public types: 8
- Translation units: 0 &mdash; every type is a value type or a pure interface

## Layout

```
app/
  include/squared/app/   one header per public type
```

No `Makefile`, because there is nothing to compile. The headers are still
covered by `tools/check_headers.sh`, which discovers modules by globbing
`*/include` rather than listing them.

## The one deliberate rule deviation

`application.hpp` names `Event` and `TextInputService` only by reference, so
the tree's forward-declare-or-include rule would make both forward
declarations. It includes them instead, because `application.hpp` is also this
module's aggregate header: `Application` cannot be implemented without both,
and there is no free name left for a separate `<module>/<module>.hpp` umbrella
with the class already holding it.

`event.hpp` is likewise the `Event` struct rather than a group umbrella. Any
consumer that included it for `KeyModifier` still compiles, because `Event`
holds a `KeyModifiers` by value and therefore includes it transitively.

## Memory

`Event` carries a `std::string text` member for `TextInput` and `TextEditing`
events, which makes it 64 bytes and heap-capable, and it is passed by const
reference everywhere &mdash; so no copy is made on the dispatch path. The
platform adapter owns the one live `Event`. `KeyModifiers` is a single
`std::uint8_t`. `TextInputArea` is four floats.

Nothing in this module allocates on its own behalf.

## Free functions

`operator|(KeyModifier, KeyModifier)` lives in `key_modifiers.hpp` rather than
a header of its own: it builds a `KeyModifiers`, so that is the type it
belongs to.
