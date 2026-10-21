# namespace detail

## Records

* [Packed](Packed.md)
* [TickClock](TickClock.md)


## Functions

### name_this_thread

*void name_this_thread(const char * name)*

 Names the calling thread, as `top -H` and a debugger show it (15 characters at most).

### not_a_valid_name

*void not_a_valid_name()*

 Not constexpr: a literal that is not a valid name calls it, and the consteval constructor below stops compiling.

### pack

*Packed pack(Verb verb, Address to, const V & value)*

*Defined at [messaging/telegraph.hpp:72](../../../../../messaging/telegraph.hpp#L72)*

