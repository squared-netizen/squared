# namespace data

## Records

* [JsonError](JsonError.md)
* [JsonParseOptions](JsonParseOptions.md)
* [JsonParseResult](JsonParseResult.md)
* [JsonValue](JsonValue.md)
* [JsonWriteOptions](JsonWriteOptions.md)
* [JsonWriteResult](JsonWriteResult.md)


## Functions

### parse_json

*JsonParseResult parse_json(int text, const JsonParseOptions & options)*



**brief** Parse exactly one strict RFC 8259 JSON document.

 Non-standard comments, trailing commas, BOMs, single-quoted strings, non-finite numbers, and invalid UTF-8 are rejected. Duplicate keys are rejected by default. When explicitly allowed, the last value wins.

### write_json

*JsonWriteResult write_json(const JsonValue & value, const JsonWriteOptions & options)*



**brief** Serialize with stable object-key ordering and number types.

 Pretty output uses two-space indentation. Unicode remains UTF-8 rather than being unnecessarily escaped.

## Enums

| enum class JsonErrorCode |

--

| None |
| InputTooLarge |
| Syntax |
| DuplicateKey |
| NestingTooDeep |
| NonFiniteNumber |
| AllocationFailure |
| InvalidValue |


*Defined at [data/src/json_error_code.hpp:9](../../../../data/src/json_error_code.hpp#L9)*



**brief** Stable error categories produced by JSON parsing and writing. 

