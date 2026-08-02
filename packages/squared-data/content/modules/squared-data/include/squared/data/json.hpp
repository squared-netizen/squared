#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace squared::data {

/**
 * @brief One owned JSON value independent of the parser backend.
 *
 * Objects use bytewise UTF-8 key ordering. This makes serialization stable
 * across runs while retaining distinct signed integers, unsigned integers,
 * and floating-point numbers.
 */
class JsonValue final {
public:
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue, std::less<>>;

    enum class Type {
        Null,
        Boolean,
        SignedInteger,
        UnsignedInteger,
        Real,
        String,
        Array,
        Object
    };

    JsonValue() noexcept = default;
    JsonValue(std::nullptr_t) noexcept;
    JsonValue(bool value) noexcept;
    JsonValue(std::int64_t value) noexcept;
    JsonValue(std::uint64_t value) noexcept;
    JsonValue(double value) noexcept;
    JsonValue(std::string value);
    JsonValue(std::string_view value);
    JsonValue(const char* value);
    JsonValue(Array value);
    JsonValue(Object value);

    /** @brief Return the exact stored JSON type. */
    [[nodiscard]] Type type() const noexcept;

    /** @brief Return whether this value stores JSON null. */
    [[nodiscard]] bool is_null() const noexcept;

    /** @brief Return the stored boolean, or null when the type differs. */
    [[nodiscard]] const bool* boolean_if() const noexcept;

    /** @brief Return the stored signed integer, or null when it differs. */
    [[nodiscard]] const std::int64_t* signed_integer_if() const noexcept;

    /** @brief Return the stored unsigned integer, or null when it differs. */
    [[nodiscard]] const std::uint64_t* unsigned_integer_if() const noexcept;

    /** @brief Return the stored real number, or null when the type differs. */
    [[nodiscard]] const double* real_if() const noexcept;

    /** @brief Return the stored UTF-8 string, or null when it differs. */
    [[nodiscard]] const std::string* string_if() const noexcept;

    /** @brief Return the stored array, or null when the type differs. */
    [[nodiscard]] Array* array_if() noexcept;
    [[nodiscard]] const Array* array_if() const noexcept;

    /** @brief Return the stored object, or null when the type differs. */
    [[nodiscard]] Object* object_if() noexcept;
    [[nodiscard]] const Object* object_if() const noexcept;

    /** @brief Find an object member without allocating a temporary key. */
    [[nodiscard]] JsonValue* find(std::string_view key) noexcept;
    [[nodiscard]] const JsonValue* find(std::string_view key) const noexcept;

private:
    using Storage = std::variant<
        std::nullptr_t,
        bool,
        std::int64_t,
        std::uint64_t,
        double,
        std::string,
        Array,
        Object
    >;

    Storage storage_{nullptr};
};

/** @brief Stable error categories produced by JSON parsing and writing. */
enum class JsonErrorCode {
    None,
    InputTooLarge,
    Syntax,
    DuplicateKey,
    NestingTooDeep,
    NonFiniteNumber,
    AllocationFailure,
    InvalidValue
};

/** @brief Structured JSON failure information. */
struct JsonError {
    JsonErrorCode code{JsonErrorCode::None};
    std::string message;
    std::size_t byte_offset{0};
    std::size_t line{0};
    std::size_t column{0};

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return code != JsonErrorCode::None;
    }
};

/** @brief Limits and strictness applied to one parse operation. */
struct JsonParseOptions {
    std::size_t maximum_bytes{8U * 1024U * 1024U};
    std::size_t maximum_depth{128};
    bool reject_duplicate_keys{true};
};

/** @brief Result of parsing one complete RFC 8259 JSON document. */
struct JsonParseResult {
    JsonValue value;
    JsonError error;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return !error;
    }
};

/** @brief Formatting controls for deterministic JSON serialization. */
struct JsonWriteOptions {
    bool pretty{false};
    bool newline_at_end{false};
};

/** @brief Result of serializing one owned JSON value. */
struct JsonWriteResult {
    std::string text;
    JsonError error;

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return !error;
    }
};

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

}  // namespace squared::data
