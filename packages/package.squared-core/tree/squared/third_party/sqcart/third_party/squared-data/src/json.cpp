module;

#include <yyjson.h>

module sq.data;

import std;

namespace sq::data
{

namespace
{

/** @brief Frees what yyjson allocated with malloc. */
struct MallocFree
{
        void operator()( char *text ) const noexcept
        {
            // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory): yyjson's allocator is malloc; this deleter is the owner
            std::free( text );
        }
};

JsonError make_error( JsonErrorCode code, std::string message )
{
    JsonError error;
    error.code = code;
    error.message = std::move( message );
    return error;
}

JsonError make_parse_error( std::string_view input, const yyjson_read_err &source )
{
    JsonError error;
    error.code = JsonErrorCode::Syntax;
    error.message = source.msg ? source.msg : "invalid JSON";
    error.byte_offset = source.pos;
    yyjson_locate_pos( input.data(), input.size(), source.pos, &error.line, &error.column,
                       nullptr );
    return error;
}

/**
 * @brief 2^53 - 1: the largest integer I-JSON allows (RFC 7493 section 2.2),
 * and the largest every double above it cannot be told apart from its
 * neighbour.
 */
constexpr std::uint64_t EXACT_INTEGER_LIMIT = ( std::uint64_t{ 1 } << 53U ) - 1;

/** @brief Whether a signed integer is within +/-(2^53 - 1). */
constexpr bool exact_integer( std::int64_t number ) noexcept
{
    return number >= -static_cast<std::int64_t>( EXACT_INTEGER_LIMIT ) &&
           number <= static_cast<std::int64_t>( EXACT_INTEGER_LIMIT );
}

bool convert_value( yyjson_val *source, std::size_t depth, const JsonParseOptions &options,
                    JsonValue &destination, JsonError &error )
{
    if( depth > options.maximum_depth ) {
        error = make_error( JsonErrorCode::NestingTooDeep,
                            "JSON nesting exceeds configured maximum depth" );
        return false;
    }

    if( yyjson_is_null( source ) ) {
        destination = JsonValue{};
        return true;
    }
    if( yyjson_is_bool( source ) ) {
        destination = JsonValue{ yyjson_get_bool( source ) };
        return true;
    }
    if( yyjson_is_uint( source ) ) {
        const std::uint64_t number = yyjson_get_uint( source );
        if( options.ijson && number > EXACT_INTEGER_LIMIT ) {
            error = make_error( JsonErrorCode::IntegerOutOfRange,
                                "JSON integer beyond 2^53 - 1 (I-JSON: write it as a string)" );
            return false;
        }
        destination = JsonValue{ number };
        return true;
    }
    if( yyjson_is_sint( source ) ) {
        const std::int64_t number = yyjson_get_sint( source );
        if( options.ijson && !exact_integer( number ) ) {
            error = make_error( JsonErrorCode::IntegerOutOfRange,
                                "JSON integer beyond -(2^53 - 1) (I-JSON: write it as a string)" );
            return false;
        }
        destination = JsonValue{ number };
        return true;
    }
    if( yyjson_is_real( source ) ) {
        const double number = yyjson_get_real( source );
        if( !std::isfinite( number ) ) {
            error = make_error( JsonErrorCode::NonFiniteNumber, "JSON real number is not finite" );
            return false;
        }
        destination = JsonValue{ number };
        return true;
    }
    if( yyjson_is_str( source ) ) {
        destination =
            JsonValue{ std::string{ yyjson_get_str( source ), yyjson_get_len( source ) } };
        return true;
    }
    if( yyjson_is_arr( source ) ) {
        JsonValue::Array array;
        array.reserve( yyjson_arr_size( source ) );
        std::size_t index;
        std::size_t maximum;
        yyjson_val *item;
        yyjson_arr_foreach( source, index, maximum, item )
        {
            JsonValue converted;
            if( !convert_value( item, depth + 1, options, converted, error ) ) {
                return false;
            }
            array.push_back( std::move( converted ) );
        }
        destination = JsonValue{ std::move( array ) };
        return true;
    }
    if( yyjson_is_obj( source ) ) {
        JsonValue::Object object;
        std::unordered_set<std::string> keys;
        keys.reserve( yyjson_obj_size( source ) );

        std::size_t index;
        std::size_t maximum;
        yyjson_val *key;
        yyjson_val *item;
        yyjson_obj_foreach( source, index, maximum, key, item )
        {
            std::string name{ yyjson_get_str( key ), yyjson_get_len( key ) };
            const bool inserted = keys.emplace( name ).second;
            if( !inserted && options.reject_duplicate_keys ) {
                error =
                    make_error( JsonErrorCode::DuplicateKey, "duplicate JSON object key: " + name );
                return false;
            }

            JsonValue converted;
            if( !convert_value( item, depth + 1, options, converted, error ) ) {
                return false;
            }
            object.insert_or_assign( std::move( name ), std::move( converted ) );
        }
        destination = JsonValue{ std::move( object ) };
        return true;
    }

    error =
        make_error( JsonErrorCode::InvalidValue, "JSON parser returned an unsupported value type" );
    return false;
}

yyjson_mut_val *convert_for_writing( yyjson_mut_doc *document, const JsonValue &source,
                                     JsonError &error )
{
    switch( source.type() ) {
        case JsonValue::Type::Null:
            return yyjson_mut_null( document );
        case JsonValue::Type::Boolean:
            return yyjson_mut_bool( document, *source.boolean_if() );
        case JsonValue::Type::SignedInteger:
            return yyjson_mut_sint( document, *source.signed_integer_if() );
        case JsonValue::Type::UnsignedInteger:
            return yyjson_mut_uint( document, *source.unsigned_integer_if() );
        case JsonValue::Type::Real: {
            const double number = *source.real_if();
            if( !std::isfinite( number ) ) {
                error = make_error( JsonErrorCode::NonFiniteNumber,
                                    "cannot write a non-finite JSON number" );
                return nullptr;
            }
            return yyjson_mut_real( document, number );
        }
        case JsonValue::Type::String: {
            const std::string &string = *source.string_if();
            return yyjson_mut_strncpy( document, string.data(), string.size() );
        }
        case JsonValue::Type::Array: {
            yyjson_mut_val *array = yyjson_mut_arr( document );
            if( !array ) {
                return nullptr;
            }
            for( const JsonValue &item : *source.array_if() ) {
                yyjson_mut_val *converted = convert_for_writing( document, item, error );
                if( !converted || !yyjson_mut_arr_append( array, converted ) ) {
                    return nullptr;
                }
            }
            return array;
        }
        case JsonValue::Type::Object: {
            yyjson_mut_val *object = yyjson_mut_obj( document );
            if( !object ) {
                return nullptr;
            }
            for( const auto &[name, item] : *source.object_if() ) {
                yyjson_mut_val *key = yyjson_mut_strncpy( document, name.data(), name.size() );
                yyjson_mut_val *converted = convert_for_writing( document, item, error );
                if( !key || !converted || !yyjson_mut_obj_add( object, key, converted ) ) {
                    return nullptr;
                }
            }
            return object;
        }
    }

    error = make_error( JsonErrorCode::InvalidValue, "cannot write an unknown JSON value type" );
    return nullptr;
}

/** @brief Append a string as RFC 8785 writes it: the fewest escapes. */
void append_canonical_string( std::string &out, std::string_view text )
{
    out += '"';
    for( const char character : text ) {
        const auto code = static_cast<unsigned char>( character );
        switch( code ) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if( code < 0x20U ) {
                    out += std::format( "\\u{:04x}", code );
                } else {
                    out += character;
                }
        }
    }
    out += '"';
}

/**
 * @brief Append a double as ECMAScript's Number.prototype.toString writes it
 * (RFC 8785 section 3.2.2.3): the shortest digits that read back as it.
 */
void append_canonical_number( std::string &out, double number )
{
    if( number == 0.0 ) {
        out += '0';
        return;
    }
    if( number < 0.0 ) {
        out += '-';
        number = -number;
    }
    std::array<char, 64> buffer{};
    const auto written = std::to_chars( buffer.data(), buffer.data() + buffer.size(), number,
                                        std::chars_format::scientific );
    // d[.ddd]e(+|-)XX
    const std::string_view scientific( buffer.data(),
                                       static_cast<std::size_t>( written.ptr - buffer.data() ) );
    const std::size_t exponent_at = scientific.find( 'e' );
    std::string digits;
    for( const char character : scientific.substr( 0, exponent_at ) ) {
        if( character != '.' ) {
            digits += character;
        }
    }
    const char *exponent_first = scientific.data() + exponent_at + 1;
    if( *exponent_first == '+' ) {
        ++exponent_first;
    }
    int exponent = 0;
    std::from_chars( exponent_first, scientific.data() + scientific.size(), exponent );
    const int count = static_cast<int>( digits.size() );
    // The value is 0.digits times 10^point.
    const int point = exponent + 1;
    if( count <= point && point <= 21 ) {
        out += digits;
        out.append( static_cast<std::size_t>( point - count ), '0' );
    } else if( 0 < point && point <= 21 ) {
        out += digits.substr( 0, static_cast<std::size_t>( point ) );
        out += '.';
        out += digits.substr( static_cast<std::size_t>( point ) );
    } else if( -6 < point && point <= 0 ) {
        out += "0.";
        out.append( static_cast<std::size_t>( -point ), '0' );
        out += digits;
    } else {
        out += digits.front();
        if( count > 1 ) {
            out += '.';
            out += digits.substr( 1 );
        }
        out += std::format( "e{}{}", point - 1 >= 0 ? '+' : '-', std::abs( point - 1 ) );
    }
}

/** @brief A member name's UTF-16 code units, the order RFC 8785 sorts by. */
std::u16string utf16_units( std::string_view text )
{
    std::u16string units;
    std::size_t at = 0;
    while( at < text.size() ) {
        const auto lead = static_cast<unsigned char>( text[at] );
        std::size_t length = 1;
        char32_t point = lead;
        if( lead >= 0xF0U ) {
            length = 4;
            point = lead & 0x07U;
        } else if( lead >= 0xE0U ) {
            length = 3;
            point = lead & 0x0FU;
        } else if( lead >= 0xC0U ) {
            length = 2;
            point = lead & 0x1FU;
        }
        for( std::size_t next = 1; next < length && at + next < text.size(); ++next ) {
            point = ( point << 6U ) | ( static_cast<unsigned char>( text[at + next] ) & 0x3FU );
        }
        at += length;
        if( point >= 0x10000U ) {
            point -= 0x10000U;
            units.push_back( static_cast<char16_t>( 0xD800U + ( point >> 10U ) ) );
            units.push_back( static_cast<char16_t>( 0xDC00U + ( point & 0x3FFU ) ) );
        } else {
            units.push_back( static_cast<char16_t>( point ) );
        }
    }
    return units;
}

/** @brief Append a value in RFC 8785 form; false, with error set, if it cannot be. */
bool append_canonical( std::string &out, const JsonValue &value, JsonError &error )
{
    switch( value.type() ) {
        case JsonValue::Type::Null:
            out += "null";
            return true;
        case JsonValue::Type::Boolean:
            out += *value.boolean_if() ? "true" : "false";
            return true;
        case JsonValue::Type::SignedInteger: {
            const std::int64_t number = *value.signed_integer_if();
            if( !exact_integer( number ) ) {
                error = make_error( JsonErrorCode::IntegerOutOfRange,
                                    "cannot write an integer beyond -2^53 canonically" );
                return false;
            }
            append_canonical_number( out, static_cast<double>( number ) );
            return true;
        }
        case JsonValue::Type::UnsignedInteger: {
            const std::uint64_t number = *value.unsigned_integer_if();
            if( number > EXACT_INTEGER_LIMIT ) {
                error = make_error( JsonErrorCode::IntegerOutOfRange,
                                    "cannot write an integer beyond 2^53 canonically" );
                return false;
            }
            append_canonical_number( out, static_cast<double>( number ) );
            return true;
        }
        case JsonValue::Type::Real: {
            const double number = *value.real_if();
            if( !std::isfinite( number ) ) {
                error = make_error( JsonErrorCode::NonFiniteNumber,
                                    "cannot write a non-finite JSON number" );
                return false;
            }
            append_canonical_number( out, number );
            return true;
        }
        case JsonValue::Type::String:
            append_canonical_string( out, *value.string_if() );
            return true;
        case JsonValue::Type::Array: {
            out += '[';
            bool first = true;
            for( const JsonValue &item : *value.array_if() ) {
                if( !first ) {
                    out += ',';
                }
                first = false;
                if( !append_canonical( out, item, error ) ) {
                    return false;
                }
            }
            out += ']';
            return true;
        }
        case JsonValue::Type::Object: {
            // Bytewise UTF-8 order (the map's) differs from UTF-16 order only
            // past U+FFFF against U+E000..U+FFFF; sort to be exact.
            std::vector<std::pair<std::u16string, const JsonValue::Object::value_type *>> members;
            for( const auto &member : *value.object_if() ) {
                members.emplace_back( utf16_units( member.first ), &member );
            }
            std::ranges::sort( members, {}, &decltype( members )::value_type::first );
            out += '{';
            bool first = true;
            for( const auto &[order, member] : members ) {
                if( !first ) {
                    out += ',';
                }
                first = false;
                append_canonical_string( out, member->first );
                out += ':';
                if( !append_canonical( out, member->second, error ) ) {
                    return false;
                }
            }
            out += '}';
            return true;
        }
    }
    error = make_error( JsonErrorCode::InvalidValue, "cannot write an unknown JSON value type" );
    return false;
}

} // namespace

JsonParseResult parse_json( std::string_view text, const JsonParseOptions &options ) noexcept
{
    JsonParseResult result;
    if( text.size() > options.maximum_bytes ) {
        result.error = make_error( JsonErrorCode::InputTooLarge,
                                   "JSON input exceeds configured maximum size" );
        return result;
    }

    yyjson_read_err read_error{};
    yyjson_doc *document = yyjson_read_opts( const_cast<char *>( text.data() ), text.size(),
                                             YYJSON_READ_NOFLAG, nullptr, &read_error );
    if( !document ) {
        result.error = make_parse_error( text, read_error );
        return result;
    }

    try {
        JsonError conversion_error;
        JsonValue converted;
        if( !convert_value( yyjson_doc_get_root( document ), 1, options, converted,
                            conversion_error ) ) {
            result.error = std::move( conversion_error );
        } else {
            result.value = std::move( converted );
        }
    } catch( const std::bad_alloc & ) {
        result.error = make_error( JsonErrorCode::AllocationFailure,
                                   "memory allocation failed while parsing JSON" );
    } catch( ... ) {
        result.error = make_error( JsonErrorCode::InvalidValue,
                                   "unexpected failure while converting parsed JSON" );
    }

    yyjson_doc_free( document );
    return result;
}

JsonWriteResult write_json( const JsonValue &value, const JsonWriteOptions &options ) noexcept
{
    JsonWriteResult result;
    if( options.canonical ) {
        if( !append_canonical( result.text, value, result.error ) ) {
            result.text.clear();
        }
        return result;
    }
    yyjson_mut_doc *document = yyjson_mut_doc_new( nullptr );
    if( !document ) {
        result.error = make_error( JsonErrorCode::AllocationFailure,
                                   "memory allocation failed while creating JSON document" );
        return result;
    }

    try {
        JsonError conversion_error;
        yyjson_mut_val *root = convert_for_writing( document, value, conversion_error );
        if( !root ) {
            result.error =
                conversion_error
                    ? std::move( conversion_error )
                    : make_error( JsonErrorCode::AllocationFailure,
                                  "memory allocation failed while building JSON output" );
            yyjson_mut_doc_free( document );
            return result;
        }
        yyjson_mut_doc_set_root( document, root );

        yyjson_write_flag flags = YYJSON_WRITE_NOFLAG;
        if( options.pretty ) {
            flags |= YYJSON_WRITE_PRETTY_TWO_SPACES;
        }
        if( options.newline_at_end ) {
            flags |= YYJSON_WRITE_NEWLINE_AT_END;
        }

        std::size_t length = 0;
        yyjson_write_err write_error{};
        char *output = yyjson_mut_write_opts( document, flags, nullptr, &length, &write_error );
        if( !output ) {
            result.error = make_error( write_error.code == YYJSON_WRITE_ERROR_NAN_OR_INF
                                           ? JsonErrorCode::NonFiniteNumber
                                           : JsonErrorCode::InvalidValue,
                                       write_error.msg ? write_error.msg : "cannot write JSON" );
        } else {
            // Owned before the copy, so a failed assign cannot leak it.
            const std::unique_ptr<char, MallocFree> owned{ output };
            result.text.assign( owned.get(), length );
        }
    } catch( const std::bad_alloc & ) {
        result.error = make_error( JsonErrorCode::AllocationFailure,
                                   "memory allocation failed while writing JSON" );
    } catch( ... ) {
        result.error =
            make_error( JsonErrorCode::InvalidValue, "unexpected failure while writing JSON" );
    }

    yyjson_mut_doc_free( document );
    return result;
}

} // namespace sq::data
