# struct Name

*Defined at [messaging/name.hpp:66](../../../../messaging/name.hpp#L66)*



**brief** The schema part of an address (`player.hp`), as its 64-bit hash.

 Eight bytes, compared and copied as one integer. A literal is hashed at compile time and checked there: `Name n = "player.hp";` costs nothing at run time, and `Name n = "Player HP";` does not compile. A name made at run time is hashed once with `Name::of`, and checked when it is registered.



## Members

public std::uint64_t id



## Functions

### Name

*public void Name()*

*Defined at [messaging/name.hpp:71](../../../../messaging/name.hpp#L71)*



**brief** No name (id 0). 

### Name

*public void Name(const char (&)[Length] text)*

*Defined at [messaging/name.hpp:76](../../../../messaging/name.hpp#L76)*

 NOLINTNEXTLINE(google-explicit-constructor): a literal is a name, as in "player.hp"

### of

*public static Name of(std::string_view text)*

*Defined at [messaging/name.hpp:85](../../../../messaging/name.hpp#L85)*



**brief** A name made at run time: hashed, not checked (NameTable::add checks). 

### from_id

*public static Name from_id(std::uint64_t id)*

*Defined at [messaging/name.hpp:93](../../../../messaging/name.hpp#L93)*



**brief** A name from its id, as stored or sent. 



