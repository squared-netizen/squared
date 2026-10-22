# namespace detail

## Records

* [Packed](Packed.md)
* [TickClock](TickClock.md)


## Functions

### earliest

*int earliest(int first, int second)*

*Defined at [messaging/model_runner.hpp:49](../../../../../messaging/model_runner.hpp#L49)*

 The earlier of two timeouts in milliseconds, -1 meaning none.

### milliseconds_until

*int milliseconds_until(time::TimePoint at, time::TimePoint now)*

 Whole milliseconds from `now` until `at`, rounded up: 0 when it has come.

### name_this_thread

*void name_this_thread(const char * name)*

 Names the calling thread, as `top -H` and a debugger show it (15 characters at most).

### not_a_valid_name

*void not_a_valid_name()*

 Not constexpr: a literal that is not a valid name calls it, and the consteval constructor below stops compiling.

### pack

*Packed pack(Verb verb, Address to, const V & value)*

*Defined at [messaging/src/pack.hpp:26](../../../../../messaging/src/pack.hpp#L26)*

