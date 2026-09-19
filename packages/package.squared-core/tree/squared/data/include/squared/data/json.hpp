#pragma once

// Aggregate header for sq::data, plus the two free
// functions that operate on JsonValue. Include a single
// type's header directly to keep a translation unit narrow.

#include <squared/data/json_error.hpp>
#include <squared/data/json_error_code.hpp>
#include <squared/data/json_parse_options.hpp>
#include <squared/data/json_parse_result.hpp>
#include <squared/data/json_value.hpp>
#include <squared/data/json_write_options.hpp>
#include <squared/data/json_write_result.hpp>

#include <string_view>

namespace sq::data {

/**
 * @brief Parse exactly one strict RFC 8259 JSON document.
 *
 * Non-standard comments, trailing commas, BOMs, single-quoted strings,
 * non-finite numbers, and invalid UTF-8 are rejected. Duplicate keys are
 * rejected by default. When explicitly allowed, the last value wins.
 */
[[nodiscard]] JsonParseResult parse_json(
    std::string_view text,
    const JsonParseOptions& options = {}
) noexcept;

/**
 * @brief Serialize with stable object-key ordering and number types.
 *
 * Pretty output uses two-space indentation. Unicode remains UTF-8 rather than
 * being unnecessarily escaped.
 */
[[nodiscard]] JsonWriteResult write_json(
    const JsonValue& value,
    const JsonWriteOptions& options = {}
) noexcept;

}  // namespace sq::data
