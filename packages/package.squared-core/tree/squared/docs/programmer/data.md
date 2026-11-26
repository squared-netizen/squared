# data

Strict RFC 8259 JSON: an owned value type independent of the parser backend,
plus one parse function and one write function. Used by the skin loader, and
for saves and layouts. And zstd: bytes compressed and decompressed, squared's
one copy of it.

Developer counterpart: [../developer/data.md](../developer/data.md)

| Type | Header | Purpose |
|---|---|---|
| `JsonValue` | `squared/data/json_value.cppm` | One owned JSON value independent of the parser backend |
| `JsonErrorCode` | `squared/data/src/json_error_code.cppm` | Stable error categories produced by JSON parsing and writing |
| `JsonError` | `squared/data/src/json_error.cppm` | Structured JSON failure information |
| `JsonParseOptions` | `squared/data/src/json_parse_options.cppm` | Limits and strictness applied to one parse operation |
| `JsonParseResult` | `squared/data/src/json_parse_result.cppm` | Result of parsing one complete RFC 8259 JSON document |
| `JsonWriteOptions` | `squared/data/src/json_write_options.cppm` | Formatting controls for deterministic JSON serialization |
| `JsonWriteResult` | `squared/data/src/json_write_result.cppm` | Result of serializing one owned JSON value |
| `ZstdResult` | `squared/data/zstd.cppm` | What `compress` or `decompress` made, or why it could not |

## Free functions

`squared/data/json.cppm` is the module's aggregate header and also declares:

| Function | Purpose |
|---|---|
| `parse_json(std::string_view, const JsonParseOptions&)` | parse exactly one strict JSON document |
| `write_json(const JsonValue&, const JsonWriteOptions&)` | serialize with stable key ordering |

Both are `noexcept` and report failure through the returned result, so this
module already follows the framework's no-exceptions rule.

## I-JSON, and the canonical form

Two options for JSON that is hashed, signed or exchanged between programs
that must agree on it (sqcart's 3.0 cartridges, for one):

- `JsonParseOptions::ijson`: accept only I-JSON (RFC 7493). Duplicate keys,
  invalid UTF-8 and lone surrogates are refused anyway; this also refuses an
  integer beyond ±(2^53 − 1), which a double cannot hold exactly
  (`JsonErrorCode::IntegerOutOfRange`). Write such a number as a string.
- `JsonWriteOptions::canonical`: write the canonical form of RFC 8785 (JSON
  Canonicalization Scheme): members ordered by their names' UTF-16 code
  units, numbers as ECMAScript writes them (`1e+21`, `0.000001`, `5e-324`),
  the fewest escapes, no whitespace. The same value always gives the same
  bytes. `pretty` and `newline_at_end` are ignored, and an integer beyond
  ±(2^53 − 1) is an error.

```cpp
const auto parsed = sq::data::parse_json(text, {.ijson = true});
const auto canonical = sq::data::write_json(parsed.value, {.canonical = true});
```

## Reading a value

Accessors return a pointer that is null when the value holds a different type,
so there is no throwing alternative to guard against:

```cpp
const auto result = sq::data::parse_json(text);
if (!result.error.ok()) return;

if (const auto* object = result.value.object_if()) {
    if (const auto* name = object->find("name")->string_if()) {
        use(*name);
    }
}
```

## yyjson

`parse_json` and `write_json` are implemented over yyjson, but nothing in the
public API exposes it: `JsonValue` owns its own storage and outlives the parse.
Only `data/src/json.cpp` includes `yyjson.h`.

## zstd

```cpp
const sq::data::ZstdResult packed = sq::data::compress(bytes);   // one frame, level 3
const sq::data::ZstdResult plain = sq::data::decompress(packed.bytes);
if (!plain) { log(plain.error); }
```

`compress` makes one zstd frame (`level` clamped to zstd's range);
`decompress` takes one frame or several in a row, and refuses more than
`limit` bytes (256 MiB unless told), a truncated frame, and what is not zstd.

squared carries one copy of zstd 1.5.7 (`third_party/zstd`), the library
`squared.zstd`, which `sq.data` links. What wants more than two functions
(streaming, dictionaries) includes `<zstd.h>`, on the include path of every
library that depends on it, and links the same one; it carries no copy of
its own. `sq.cart` is given it too (`sqcart_settings.zstd`), so an app with
Settings, whose preferences are cartridges, links zstd once. Only
`data/src/zstd.cpp` includes it in `sq.data`, an object of its own: an app
that never calls these links none of zstd.

