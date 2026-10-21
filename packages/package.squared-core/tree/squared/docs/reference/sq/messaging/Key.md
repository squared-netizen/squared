# class Key

*Defined at [messaging/key.hpp:39](../../../../messaging/key.hpp#L39)*



**brief** An address in code: its name, the type of its value, the verbs it carries, and whether it takes a uid.

 ```cpp inline constexpr sq::Key<std::int64_t, sq::state>          ticks{ "counter.ticks" }; inline constexpr sq::Key<bool, sq::state | sq::set>        sound{ "settings.sound" }; inline constexpr sq::Key<std::string_view, sq::state, sq::with_uid> item_name{ "item.name" }; ```

 The only way to name an address in an app. Written by hand or generated from `names.json`, it is the same type. The type, the verbs and the uid flag exist for the compiler: sending the wrong type, a verb the address does not carry, or no uid where one is needed does not compile. What is kept is the name (8 bytes) and a pointer to its text (8 bytes), for the name table; a telegram carries only the name's id.



**T** One of the value types (`is_value_type`); `void` for none.



## Members

public static Verbs verbs

public static const bool takes_uid

public static ValueType type



## Functions

### Key<T, V, Uid>

*public void Key<T, V, Uid>(const char (&)[Length] text)*

*Defined at [messaging/key.hpp:52](../../../../messaging/key.hpp#L52)*

 NOLINTNEXTLINE(google-explicit-constructor): a literal is a key, as in "counter.ticks"

### name

*public Name name()*

*Defined at [messaging/key.hpp:58](../../../../messaging/key.hpp#L58)*



**brief** The name, as a telegram carries it. 

### text

*public std::string_view text()*

*Defined at [messaging/key.hpp:61](../../../../messaging/key.hpp#L61)*



**brief** The name's text, for the name table. 



