// Aggregate header for sq::data, plus the two free
// functions that operate on JsonValue. Include a single
// type's header directly to keep a translation unit narrow.

export module sq.data:json;

export import :json_value;
export import :src.json_error;
export import :src.json_error_code;
export import :src.json_parse_options;
export import :src.json_parse_result;
export import :src.json_write_options;
export import :src.json_write_result;
import std;

export namespace sq::data
{

/**
 * @brief Parse exactly one strict RFC 8259 JSON document.
 *
 * Non-standard comments, trailing commas, BOMs, single-quoted strings,
 * non-finite numbers, and invalid UTF-8 are rejected. Duplicate keys are
 * rejected by default. When explicitly allowed, the last value wins.
 */
[[nodiscard]] JsonParseResult parse_json( std::string_view text,
                                          const JsonParseOptions &options = {} ) noexcept;

/**
 * @brief Serialize with stable object-key ordering and number types.
 *
 * Pretty output uses two-space indentation. Unicode remains UTF-8 rather than
 * being unnecessarily escaped.
 */
[[nodiscard]] JsonWriteResult write_json( const JsonValue &value,
                                          const JsonWriteOptions &options = {} ) noexcept;

} // namespace sq::data
