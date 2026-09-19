#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace sq::data {

/**
 * @brief One owned JSON value independent of the parser backend.
 *
 * Objects use bytewise UTF-8 key ordering. This makes serialization stable
 * across runs while retaining distinct signed integers, unsigned integers,
 * and floating-point numbers.
 */
class JsonValue final {
public:
    /** @brief JSON value container. */
    using Array = std::vector<JsonValue>;

    /** @brief Object with bytewise-ordered UTF-8 keys. */
    using Object = std::map<std::string, JsonValue, std::less<>>;

    /** @brief Exact stored JSON type. */
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

    /** @brief Default-construct a JSON null value. */
    JsonValue() noexcept = default;

    /** @brief Construct a JSON null value. */
    JsonValue(std::nullptr_t) noexcept;

    /**
     * @brief Construct a JSON boolean.
     * @param value Boolean to store.
     */
    JsonValue(bool value) noexcept;

    /**
     * @brief Construct a JSON signed integer.
     * @param value Signed 64-bit integer to store.
     */
    JsonValue(std::int64_t value) noexcept;

    /**
     * @brief Construct a JSON unsigned integer.
     * @param value Unsigned 64-bit integer to store.
     */
    JsonValue(std::uint64_t value) noexcept;

    /**
     * @brief Construct a JSON real number.
     * @param value Finite or non-finite double to store; writing a non-finite
     * value yields a JsonError.
     */
    JsonValue(double value) noexcept;

    /**
     * @brief Construct a JSON string, copying the value.
     * @param value UTF-8 text to copy into the stored value.
     */
    JsonValue(std::string value);

    /**
     * @brief Construct a JSON string, copying the view.
     * @param value UTF-8 text to copy into the stored value.
     */
    JsonValue(std::string_view value);

    /**
     * @brief Construct a JSON string from a null-terminated value.
     * @param value Null-terminated UTF-8 text to copy.
     */
    JsonValue(const char* value);

    /**
     * @brief Construct a JSON array by moving the vector.
     * @param value Array whose storage is moved into the value.
     */
    JsonValue(Array value);

    /**
     * @brief Construct a JSON object by moving the map.
     * @param value Object whose storage is moved into the value.
     */
    JsonValue(Object value);

    /**
     * @brief Return the exact stored JSON type.
     * @return The Type of the stored value or Type::Null.
     */
    [[nodiscard]] Type type() const noexcept;

    /**
     * @brief Return whether this value stores JSON null.
     * @return `true` when type() is Type::Null.
     */
    [[nodiscard]] bool is_null() const noexcept;

    /**
     * @brief Return the stored boolean, or null when the type differs.
     * @return Pointer to the stored bool, or null.
     */
    [[nodiscard]] const bool* boolean_if() const noexcept;

    /**
     * @brief Return the stored signed integer, or null when it differs.
     * @return Pointer to the stored std::int64_t, or null.
     */
    [[nodiscard]] const std::int64_t* signed_integer_if() const noexcept;

    /**
     * @brief Return the stored unsigned integer, or null when it differs.
     * @return Pointer to the stored std::uint64_t, or null.
     */
    [[nodiscard]] const std::uint64_t* unsigned_integer_if() const noexcept;

    /**
     * @brief Return the stored real number, or null when the type differs.
     * @return Pointer to the stored double, or null.
     */
    [[nodiscard]] const double* real_if() const noexcept;

    /**
     * @brief Return the stored UTF-8 string, or null when it differs.
     * @return Pointer to the stored std::string, or null.
     */
    [[nodiscard]] const std::string* string_if() const noexcept;

    /**
     * @brief Return the stored array, or null when the type differs.
     * @return Pointer to the stored array, or null.
     */
    [[nodiscard]] Array* array_if() noexcept;

    /**
     * @brief Return the stored array, read-only.
     * @return Pointer to the stored array, or null when the type differs.
     */
    [[nodiscard]] const Array* array_if() const noexcept;

    /**
     * @brief Return the stored object, or null when the type differs.
     * @return Pointer to the stored object, or null.
     */
    [[nodiscard]] Object* object_if() noexcept;

    /**
     * @brief Return the stored object, read-only.
     * @return Pointer to the stored object, or null when the type differs.
     */
    [[nodiscard]] const Object* object_if() const noexcept;

    /**
     * @brief Find an object member without allocating a temporary key.
     * @param key Member name to look up.
     * @return Pointer to the member value, or null when the value is not an
     * object or the key is absent.
     */
    [[nodiscard]] JsonValue* find(std::string_view key) noexcept;

    /**
     * @brief Find an object member, read-only.
     * @param key Member name to look up.
     * @return Pointer to the member value, or null when the value is not an
     * object or the key is absent.
     */
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

}  // namespace sq::data
