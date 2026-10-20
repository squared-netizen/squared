# namespace detail

## Functions

### apk_of_library

*bool apk_of_library(int library, char * out, int size)*



**brief** The APK a library was loaded from, from the library's path as the loader reports it: `<apk>!/lib/<abi>/<name>.so` when it was loaded from inside the APK, `<dir>/lib/<abi>/<name>.so` when it was extracted beside `<dir>/base.apk`. Writes a terminated path into `out`.

**return** False for any other path, or when `out` is too small. Exposed for tests.

### characters

*int characters(int text)*

*Defined at [console/layout.hpp:106](../../../../../console/layout.hpp#L106)*

### colour_of

*Colour colour_of(Level level)*

*Defined at [console/layout.hpp:115](../../../../../console/layout.hpp#L115)*

### glyph_at

*int glyph_at(int text, int & at)*

*Defined at [console/layout.hpp:94](../../../../../console/layout.hpp#L94)*

 The glyph for the character starting at text[at], and the start of the next.

