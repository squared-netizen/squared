# class JsonValue

*Defined at [data/json_value.hpp:21](../../../../data/json_value.hpp#L21)*



**brief** One owned JSON value independent of the parser backend.

 Objects use bytewise UTF-8 key ordering. This makes serialization stable across runs while retaining distinct signed integers, unsigned integers, and floating-point numbers.



## Functions

### JsonValue

*public void JsonValue()*

*Defined at [data/json_value.hpp:43](../../../../data/json_value.hpp#L43)*



**brief** Default-construct a JSON null value. 

### JsonValue

*public void JsonValue(std::nullptr_t )*



**brief** Construct a JSON null value. 

### JsonValue

*public void JsonValue(bool value)*



**brief** Construct a JSON boolean.

**value** Boolean to store.

### JsonValue

*public void JsonValue(std::int64_t value)*



**brief** Construct a JSON signed integer.

**value** Signed 64-bit integer to store.

### JsonValue

*public void JsonValue(std::uint64_t value)*



**brief** Construct a JSON unsigned integer.

**value** Unsigned 64-bit integer to store.

### JsonValue

*public void JsonValue(double value)*



**brief** Construct a JSON real number.

**value** Finite or non-finite double to store; writing a non-finite value yields a JsonError.

### JsonValue

*public void JsonValue(std::string value)*



**brief** Construct a JSON string, copying the value.

**value** UTF-8 text to copy into the stored value.

### JsonValue

*public void JsonValue(std::string_view value)*



**brief** Construct a JSON string, copying the view.

**value** UTF-8 text to copy into the stored value.

### JsonValue

*public void JsonValue(const char * value)*



**brief** Construct a JSON string from a null-terminated value.

**value** Null-terminated UTF-8 text to copy.

### JsonValue

*public void JsonValue(Array value)*



**brief** Construct a JSON array by moving the vector.

**value** Array whose storage is moved into the value.

### JsonValue

*public void JsonValue(Object value)*



**brief** Construct a JSON object by moving the map.

**value** Object whose storage is moved into the value.

### type

*public Type type()*



**brief** Return the exact stored JSON type.

**return** The Type of the stored value or Type::Null.

### is_null

*public bool is_null()*



**brief** Return whether this value stores JSON null.

**return** `true` when type() is Type::Null.

### boolean_if

*public const bool * boolean_if()*



**brief** Return the stored boolean, or null when the type differs.

**return** Pointer to the stored bool, or null.

### signed_integer_if

*public const std::int64_t * signed_integer_if()*



**brief** Return the stored signed integer, or null when it differs.

**return** Pointer to the stored std::int64_t, or null.

### unsigned_integer_if

*public const std::uint64_t * unsigned_integer_if()*



**brief** Return the stored unsigned integer, or null when it differs.

**return** Pointer to the stored std::uint64_t, or null.

### real_if

*public const double * real_if()*



**brief** Return the stored real number, or null when the type differs.

**return** Pointer to the stored double, or null.

### string_if

*public const std::string * string_if()*



**brief** Return the stored UTF-8 string, or null when it differs.

**return** Pointer to the stored std::string, or null.

### array_if

*public Array * array_if()*



**brief** Return the stored array, or null when the type differs.

**return** Pointer to the stored array, or null.

### array_if

*public const Array * array_if()*



**brief** Return the stored array, read-only.

**return** Pointer to the stored array, or null when the type differs.

### object_if

*public Object * object_if()*



**brief** Return the stored object, or null when the type differs.

**return** Pointer to the stored object, or null.

### object_if

*public const Object * object_if()*



**brief** Return the stored object, read-only.

**return** Pointer to the stored object, or null when the type differs.

### find

*public JsonValue * find(std::string_view key)*



**brief** Find an object member without allocating a temporary key.

**key** Member name to look up.

**return** Pointer to the member value, or null when the value is not an object or the key is absent.

### find

*public const JsonValue * find(std::string_view key)*



**brief** Find an object member, read-only.

**key** Member name to look up.

**return** Pointer to the member value, or null when the value is not an object or the key is absent.



## Enums

| enum class Type |

--

| Null |
| Boolean |
| SignedInteger |
| UnsignedInteger |
| Real |
| String |
| Array |
| Object |


*Defined at [data/json_value.hpp:31](../../../../data/json_value.hpp#L31)*



**brief** Exact stored JSON type. 



