# namespace detail

## Functions

### apk_of_library

*bool apk_of_library(std::string_view library, char * out, std::size_t size)*



**brief** The APK a library was loaded from, from the library's path as the loader reports it: `<apk>!/lib/<abi>/<name>.so` when it was loaded from inside the APK, `<dir>/lib/<abi>/<name>.so` when it was extracted beside `<dir>/base.apk`. Writes a terminated path into `out`.

**return** False for any other path, or when `out` is too small. Exposed for tests.

### characters

*std::size_t characters(std::string_view text)*

*Defined at [console/layout.cppm:102](../../../../../console/layout.cppm#L102)*

### colour_of

*Colour colour_of(Level level)*

*Defined at [console/layout.cppm:111](../../../../../console/layout.cppm#L111)*

### glyph_at

*std::uint8_t glyph_at(std::string_view text, std::size_t & at)*

*Defined at [console/layout.cppm:90](../../../../../console/layout.cppm#L90)*

 The glyph for the character starting at text[at], and the start of the next.

