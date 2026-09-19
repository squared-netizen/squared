# data

Strict RFC 8259 JSON: an owned value type independent of the parser backend,
plus one parse function and one write function. Used by the skin loader and by
`sq::messaging` for telegram payloads.

Developer counterpart: [../developer/data.md](../developer/data.md)

| Type | Header | Purpose |
|---|---|---|
| `JsonValue` | `squared/data/json_value.hpp` | One owned JSON value independent of the parser backend |
| `JsonErrorCode` | `squared/data/json_error_code.hpp` | Stable error categories produced by JSON parsing and writing |
| `JsonError` | `squared/data/json_error.hpp` | Structured JSON failure information |
| `JsonParseOptions` | `squared/data/json_parse_options.hpp` | Limits and strictness applied to one parse operation |
| `JsonParseResult` | `squared/data/json_parse_result.hpp` | Result of parsing one complete RFC 8259 JSON document |
| `JsonWriteOptions` | `squared/data/json_write_options.hpp` | Formatting controls for deterministic JSON serialization |
| `JsonWriteResult` | `squared/data/json_write_result.hpp` | Result of serializing one owned JSON value |

## Free functions

`squared/data/json.hpp` is the module's aggregate header and also declares:

| Function | Purpose |
|---|---|
| `parse_json(std::string_view, const JsonParseOptions&)` | parse exactly one strict JSON document |
| `write_json(const JsonValue&, const JsonWriteOptions&)` | serialize with stable key ordering |

Both are `noexcept` and report failure through the returned result, so this
module already follows the framework's no-exceptions rule.

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
