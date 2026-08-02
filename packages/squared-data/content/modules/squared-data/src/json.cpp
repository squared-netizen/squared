#include <squared/data/json.hpp>

#include <yyjson.h>

#include <cmath>
#include <cstdlib>
#include <new>
#include <string>
#include <unordered_set>
#include <utility>

namespace squared::data {
namespace {

JsonError make_error(JsonErrorCode code, std::string message)
{
    JsonError error;
    error.code = code;
    error.message = std::move(message);
    return error;
}

JsonError make_parse_error(
    std::string_view input,
    const yyjson_read_err& source
)
{
    JsonError error;
    error.code = JsonErrorCode::Syntax;
    error.message = source.msg ? source.msg : "invalid JSON";
    error.byte_offset = source.pos;
    yyjson_locate_pos(
        input.data(),
        input.size(),
        source.pos,
        &error.line,
        &error.column,
        nullptr
    );
    return error;
}

bool convert_value(
    yyjson_val* source,
    std::size_t depth,
    const JsonParseOptions& options,
    JsonValue& destination,
    JsonError& error
)
{
    if (depth > options.maximum_depth) {
        error = make_error(
            JsonErrorCode::NestingTooDeep,
            "JSON nesting exceeds configured maximum depth"
        );
        return false;
    }

    if (yyjson_is_null(source)) {
        destination = JsonValue{};
        return true;
    }
    if (yyjson_is_bool(source)) {
        destination = JsonValue{yyjson_get_bool(source)};
        return true;
    }
    if (yyjson_is_uint(source)) {
        destination = JsonValue{yyjson_get_uint(source)};
        return true;
    }
    if (yyjson_is_sint(source)) {
        destination = JsonValue{yyjson_get_sint(source)};
        return true;
    }
    if (yyjson_is_real(source)) {
        const double number = yyjson_get_real(source);
        if (!std::isfinite(number)) {
            error = make_error(
                JsonErrorCode::NonFiniteNumber,
                "JSON real number is not finite"
            );
            return false;
        }
        destination = JsonValue{number};
        return true;
    }
    if (yyjson_is_str(source)) {
        destination = JsonValue{std::string{
            yyjson_get_str(source),
            yyjson_get_len(source)
        }};
        return true;
    }
    if (yyjson_is_arr(source)) {
        JsonValue::Array array;
        array.reserve(yyjson_arr_size(source));
        std::size_t index;
        std::size_t maximum;
        yyjson_val* item;
        yyjson_arr_foreach(source, index, maximum, item) {
            JsonValue converted;
            if (!convert_value(
                    item,
                    depth + 1,
                    options,
                    converted,
                    error
                )) {
                return false;
            }
            array.push_back(std::move(converted));
        }
        destination = JsonValue{std::move(array)};
        return true;
    }
    if (yyjson_is_obj(source)) {
        JsonValue::Object object;
        std::unordered_set<std::string> keys;
        keys.reserve(yyjson_obj_size(source));

        std::size_t index;
        std::size_t maximum;
        yyjson_val* key;
        yyjson_val* item;
        yyjson_obj_foreach(source, index, maximum, key, item) {
            std::string name{yyjson_get_str(key), yyjson_get_len(key)};
            const bool inserted = keys.emplace(name).second;
            if (!inserted && options.reject_duplicate_keys) {
                error = make_error(
                    JsonErrorCode::DuplicateKey,
                    "duplicate JSON object key: " + name
                );
                return false;
            }

            JsonValue converted;
            if (!convert_value(
                    item,
                    depth + 1,
                    options,
                    converted,
                    error
                )) {
                return false;
            }
            object.insert_or_assign(std::move(name), std::move(converted));
        }
        destination = JsonValue{std::move(object)};
        return true;
    }

    error = make_error(
        JsonErrorCode::InvalidValue,
        "JSON parser returned an unsupported value type"
    );
    return false;
}

yyjson_mut_val* convert_for_writing(
    yyjson_mut_doc* document,
    const JsonValue& source,
    JsonError& error
)
{
    switch (source.type()) {
    case JsonValue::Type::Null:
        return yyjson_mut_null(document);
    case JsonValue::Type::Boolean:
        return yyjson_mut_bool(document, *source.boolean_if());
    case JsonValue::Type::SignedInteger:
        return yyjson_mut_sint(document, *source.signed_integer_if());
    case JsonValue::Type::UnsignedInteger:
        return yyjson_mut_uint(document, *source.unsigned_integer_if());
    case JsonValue::Type::Real: {
        const double number = *source.real_if();
        if (!std::isfinite(number)) {
            error = make_error(
                JsonErrorCode::NonFiniteNumber,
                "cannot write a non-finite JSON number"
            );
            return nullptr;
        }
        return yyjson_mut_real(document, number);
    }
    case JsonValue::Type::String: {
        const std::string& string = *source.string_if();
        return yyjson_mut_strncpy(
            document,
            string.data(),
            string.size()
        );
    }
    case JsonValue::Type::Array: {
        yyjson_mut_val* array = yyjson_mut_arr(document);
        if (!array) return nullptr;
        for (const JsonValue& item : *source.array_if()) {
            yyjson_mut_val* converted =
                convert_for_writing(document, item, error);
            if (!converted || !yyjson_mut_arr_append(array, converted)) {
                return nullptr;
            }
        }
        return array;
    }
    case JsonValue::Type::Object: {
        yyjson_mut_val* object = yyjson_mut_obj(document);
        if (!object) return nullptr;
        for (const auto& [name, item] : *source.object_if()) {
            yyjson_mut_val* key = yyjson_mut_strncpy(
                document,
                name.data(),
                name.size()
            );
            yyjson_mut_val* converted =
                convert_for_writing(document, item, error);
            if (!key ||
                !converted ||
                !yyjson_mut_obj_add(object, key, converted)) {
                return nullptr;
            }
        }
        return object;
    }
    }

    error = make_error(
        JsonErrorCode::InvalidValue,
        "cannot write an unknown JSON value type"
    );
    return nullptr;
}

}  // namespace

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

JsonParseResult parse_json(
    std::string_view text,
    const JsonParseOptions& options
) noexcept
{
    JsonParseResult result;
    if (text.size() > options.maximum_bytes) {
        result.error = make_error(
            JsonErrorCode::InputTooLarge,
            "JSON input exceeds configured maximum size"
        );
        return result;
    }

    yyjson_read_err read_error{};
    yyjson_doc* document = yyjson_read_opts(
        const_cast<char*>(text.data()),
        text.size(),
        YYJSON_READ_NOFLAG,
        nullptr,
        &read_error
    );
    if (!document) {
        result.error = make_parse_error(text, read_error);
        return result;
    }

    try {
        JsonError conversion_error;
        JsonValue converted;
        if (!convert_value(
                yyjson_doc_get_root(document),
                1,
                options,
                converted,
                conversion_error
            )) {
            result.error = std::move(conversion_error);
        } else {
            result.value = std::move(converted);
        }
    } catch (const std::bad_alloc&) {
        result.error = make_error(
            JsonErrorCode::AllocationFailure,
            "memory allocation failed while parsing JSON"
        );
    } catch (...) {
        result.error = make_error(
            JsonErrorCode::InvalidValue,
            "unexpected failure while converting parsed JSON"
        );
    }

    yyjson_doc_free(document);
    return result;
}

JsonWriteResult write_json(
    const JsonValue& value,
    const JsonWriteOptions& options
) noexcept
{
    JsonWriteResult result;
    yyjson_mut_doc* document = yyjson_mut_doc_new(nullptr);
    if (!document) {
        result.error = make_error(
            JsonErrorCode::AllocationFailure,
            "memory allocation failed while creating JSON document"
        );
        return result;
    }

    try {
        JsonError conversion_error;
        yyjson_mut_val* root =
            convert_for_writing(document, value, conversion_error);
        if (!root) {
            result.error = conversion_error
                ? std::move(conversion_error)
                : make_error(
                    JsonErrorCode::AllocationFailure,
                    "memory allocation failed while building JSON output"
                );
            yyjson_mut_doc_free(document);
            return result;
        }
        yyjson_mut_doc_set_root(document, root);

        yyjson_write_flag flags = YYJSON_WRITE_NOFLAG;
        if (options.pretty) flags |= YYJSON_WRITE_PRETTY_TWO_SPACES;
        if (options.newline_at_end) flags |= YYJSON_WRITE_NEWLINE_AT_END;

        std::size_t length = 0;
        yyjson_write_err write_error{};
        char* output = yyjson_mut_write_opts(
            document,
            flags,
            nullptr,
            &length,
            &write_error
        );
        if (!output) {
            result.error = make_error(
                write_error.code == YYJSON_WRITE_ERROR_NAN_OR_INF
                    ? JsonErrorCode::NonFiniteNumber
                    : JsonErrorCode::InvalidValue,
                write_error.msg ? write_error.msg : "cannot write JSON"
            );
        } else {
            result.text.assign(output, length);
            std::free(output);
        }
    } catch (const std::bad_alloc&) {
        result.error = make_error(
            JsonErrorCode::AllocationFailure,
            "memory allocation failed while writing JSON"
        );
    } catch (...) {
        result.error = make_error(
            JsonErrorCode::InvalidValue,
            "unexpected failure while writing JSON"
        );
    }

    yyjson_mut_doc_free(document);
    return result;
}

}  // namespace squared::data
