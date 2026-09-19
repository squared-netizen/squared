#include <squared/data/json_value.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace sq::data {

JsonValue::JsonValue(std::nullptr_t) noexcept
    : storage_(nullptr)
{
}

JsonValue::JsonValue(bool value) noexcept
    : storage_(value)
{
}

JsonValue::JsonValue(std::int64_t value) noexcept
    : storage_(value)
{
}

JsonValue::JsonValue(std::uint64_t value) noexcept
    : storage_(value)
{
}

JsonValue::JsonValue(double value) noexcept
    : storage_(value)
{
}

JsonValue::JsonValue(std::string value)
    : storage_(std::move(value))
{
}

JsonValue::JsonValue(std::string_view value)
    : storage_(std::string{value})
{
}

JsonValue::JsonValue(const char* value)
    : storage_(std::string{value ? value : ""})
{
}

JsonValue::JsonValue(Array value)
    : storage_(std::move(value))
{
}

JsonValue::JsonValue(Object value)
    : storage_(std::move(value))
{
}

JsonValue::Type JsonValue::type() const noexcept
{
    return static_cast<Type>(storage_.index());
}

bool JsonValue::is_null() const noexcept
{
    return std::holds_alternative<std::nullptr_t>(storage_);
}

const bool* JsonValue::boolean_if() const noexcept
{
    return std::get_if<bool>(&storage_);
}

const std::int64_t* JsonValue::signed_integer_if() const noexcept
{
    return std::get_if<std::int64_t>(&storage_);
}

const std::uint64_t* JsonValue::unsigned_integer_if() const noexcept
{
    return std::get_if<std::uint64_t>(&storage_);
}

const double* JsonValue::real_if() const noexcept
{
    return std::get_if<double>(&storage_);
}

const std::string* JsonValue::string_if() const noexcept
{
    return std::get_if<std::string>(&storage_);
}

JsonValue::Array* JsonValue::array_if() noexcept
{
    return std::get_if<Array>(&storage_);
}

const JsonValue::Array* JsonValue::array_if() const noexcept
{
    return std::get_if<Array>(&storage_);
}

JsonValue::Object* JsonValue::object_if() noexcept
{
    return std::get_if<Object>(&storage_);
}

const JsonValue::Object* JsonValue::object_if() const noexcept
{
    return std::get_if<Object>(&storage_);
}

JsonValue* JsonValue::find(std::string_view key) noexcept
{
    Object* object = object_if();
    if (!object) return nullptr;
    const auto found = object->find(key);
    return found == object->end() ? nullptr : &found->second;
}

const JsonValue* JsonValue::find(std::string_view key) const noexcept
{
    const Object* object = object_if();
    if (!object) return nullptr;
    const auto found = object->find(key);
    return found == object->end() ? nullptr : &found->second;
}

}  // namespace sq::data
