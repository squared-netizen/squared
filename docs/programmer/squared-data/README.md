# Squared Data — Programmer Guide

Squared Data is the framework's strict, deterministic JSON service. It
provides owned JSON values whose lifetime does not depend on any parser, a
strict RFC 8259 parser with configurable resource limits, and a deterministic
serializer. Parsed values keep their exact number category, and object keys
serialize in a stable bytewise order, which makes the module suitable for
manifests, skins, editor documents, and application persistence.

## Package availability

| Module | Version | Requires |
| --- | --- | --- |
| `dev.squarednetizen.squared.data` | `0.6.0-dev.2` | (none) |

The CMake target is `squared_data`. The module is self-contained; the parser
backend is pinned inside the package and never appears in the public API.

## Public API overview

| Type | Header | Purpose |
| --- | --- | --- |
| `squared::data::JsonValue` | `squared/data/json.hpp` | Owned JSON value with an exact stored type. |
| `JsonValue::Type` | `squared/data/json.hpp` | Enum of the exact stored JSON categories. |
| `JsonErrorCode` | `squared/data/json.hpp` | Stable failure categories for parsing and writing. |
| `JsonError` | `squared/data/json.hpp` | Structured failure with message, byte offset, line, and column. |
| `JsonParseOptions` | `squared/data/json.hpp` | Input size, nesting depth, and duplicate-key policy for one parse. |
| `JsonParseResult` | `squared/data/json.hpp` | Parsed value plus optional error. |
| `JsonWriteOptions` | `squared/data/json.hpp` | Pretty printing and trailing newline controls. |
| `JsonWriteResult` | `squared/data/json.hpp` | Serialized text plus optional error. |
| `parse_json` | `squared/data/json.hpp` | Parse exactly one strict JSON document. |
| `write_json` | `squared/data/json.hpp` | Serialize one value deterministically. |

## JsonValue and type checks

`JsonValue` is a value type holding exactly one of the JSON categories:
`Null`, `Boolean`, `SignedInteger`, `UnsignedInteger`, `Real`, `String`,
`Array`, or `Object`. `type()` reports the exact stored category; the
`*_if()` accessors return a pointer to the stored value only when the type
matches, and `nullptr` otherwise. `find(key)` locates an object member without
allocating a temporary key and returns `nullptr` when the value is not an
object or the key is absent.

```cpp
#include <squared/data/json.hpp>

#include <cstdint>
#include <string>

int main()
{
    const auto parsed = squared::data::parse_json(
        R"({"name":"torch","lit":true,"count":7,"ratio":1.25})"
    );
    if (!parsed) {
        return 1;
    }
    const squared::data::JsonValue& root = parsed.value;
    if (const auto* name = root.find("name");
        name && name->string_if()) {
        const std::string& text = *name->string_if();
    }
    if (const auto* lit = root.find("lit");
        lit && lit->boolean_if()) {
    }
    if (const auto* count = root.find("count");
        count && count->signed_integer_if()) {
        const std::int64_t value = *count->signed_integer_if();
    }
    if (const auto* ratio = root.find("ratio");
        ratio && ratio->real_if()) {
        const double value = *ratio->real_if();
    }
    return 0;
}
```

`find` returns a pointer that is valid for the lifetime of the `JsonValue`
it belongs to. Prefer `find` plus the typed `*_if()` checks; there is no
throwing conversion API.

## Parsing

`parse_json(text)` accepts one RFC 8259 document. Non-standard comments,
trailing commas, single-quoted strings, byte-order marks, invalid UTF-8, and
non-finite numbers are rejected, and duplicate object keys are errors by
default. Signed, unsigned, and real number categories are preserved exactly:
`-7` stays a `SignedInteger`, a full 64-bit unsigned value stays
`UnsignedInteger`, and `1.25` stays a `Real`.

```cpp
#include <squared/data/json.hpp>

int main()
{
    const auto parsed = squared::data::parse_json(R"({"a":1,"b":2})");
    if (!parsed) {
        return 1;  // parsed.error carries the failure
    }
    return 0;
}
```

### Parse options

`JsonParseOptions` controls resource use on one call:

- `maximum_bytes` — maximum accepted document size, default 8 MiB.
- `maximum_depth` — maximum nesting depth of arrays and objects, default 128.
- `reject_duplicate_keys` — default `true`; when set to `false` for
  compatibility imports, the last duplicate key wins.

```cpp
#include <squared/data/json.hpp>

squared::data::JsonParseResult parse_bounded(std::string_view text)
{
    squared::data::JsonParseOptions options;
    options.maximum_bytes = 4U * 1024U * 1024U;
    options.maximum_depth = 64;
    return squared::data::parse_json(text, options);
}
```

Input larger than `maximum_bytes` fails with `JsonErrorCode::InputTooLarge`
before any parsing. Nesting beyond `maximum_depth` fails with
`JsonErrorCode::NestingTooDeep`.

## Serialization

`write_json(value)` emits compact output by default. Object keys are written
in bytewise UTF-8 order, so the output is stable across runs and processes.
Number categories are retained, and Unicode text stays UTF-8 rather than being
escaped unnecessarily. Pretty output uses two-space indentation and an optional
final newline.

```cpp
#include <squared/data/json.hpp>

#include <cstdint>
#include <string>

int main()
{
    squared::data::JsonValue::Object object;
    object.emplace("z", squared::data::JsonValue{std::uint64_t{2}});
    object.emplace("a", squared::data::JsonValue{"first"});
    object.emplace("m", squared::data::JsonValue{
        squared::data::JsonValue::Array{
            squared::data::JsonValue{true},
            squared::data::JsonValue{nullptr},
            squared::data::JsonValue{std::int64_t{-3}}
        }
    });

    const squared::data::JsonValue value{std::move(object)};
    const auto compact = squared::data::write_json(value);
    // compact.text == R"({"a":"first","m":[true,null,-3],"z":2})"

    squared::data::JsonWriteOptions pretty;
    pretty.pretty = true;
    pretty.newline_at_end = true;
    const auto formatted = squared::data::write_json(value, pretty);
    return 0;
}
```

Writing a non-finite `double` (`infinity` or a NaN stored in a `JsonValue`)
fails with `JsonErrorCode::NonFiniteNumber`, and writing an invalid UTF-8
string fails. Parse and write round trips are stable: writing the parsed
value reproduces the same deterministic text.

## Errors and failure behavior

`parse_json` and `write_json` never throw. They return a result holding an
empty error on success and a structured `JsonError` on failure:

- `code` — stable `JsonErrorCode` category.
- `message` — human-readable diagnostic.
- `byte_offset` — zero-based byte offset into the failing document (parse only).
- `line` and `column` — one-based source position of the failure (parse only).

`JsonErrorCode` categories:

| Code | Meaning |
| --- | --- |
| `InputTooLarge` | Document exceeds `maximum_bytes`. |
| `Syntax` | Document is not valid strict RFC 8259 JSON. |
| `DuplicateKey` | Duplicate object key found while duplicates are rejected. |
| `NestingTooDeep` | Array/object nesting exceeds `maximum_depth`. |
| `NonFiniteNumber` | Non-finite number encountered (parse rejection or write refusal). |
| `AllocationFailure` | Memory allocation failed during the operation. |
| `InvalidValue` | Value cannot be represented or written as JSON. |

A `JsonError` with code `None` is not an error; both result types convert to
`bool` accordingly. On failure the `JsonParseResult::value` and
`JsonWriteResult::text` are not meaningful.

## Ownership and lifetime

`JsonValue` is a value type over a discriminated union. Copy freely; copying
deep-copies the contained value, so no `JsonValue` aliases another's storage
and there is no dangling risk. Pointers returned by `find` and the `*_if()`
accessors remain valid only for the lifetime of the `JsonValue` they belong
to. `Array` is `std::vector<JsonValue>` and `Object` is a
`std::map<std::string, JsonValue>`; both can be moved into a `JsonValue`
without a copy.

## Threading

The free functions are independent and share no global state; distinct
`JsonValue` instances can be read from different threads. A single `JsonValue`
being mutated concurrently by multiple threads must be externally
synchronized, as with any standard library container.

## Lua bindings

None of the types or functions in this package have a Lua 5.4 binding.

## Related documentation

- Package payload: [Data.md](../../../packages/squared-data/content/docs/Data.md)
- Implementation details: [Squared Data — Developer Guide](../developer/squared-data/README.md)
- Documentation index: [Programmer documentation](../README.md)
