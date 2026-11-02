# namespace detail

## Records

* [DecodedImage](DecodedImage.md)


## Functions

### decode_image

*DecodedImage decode_image(std::span<const std::byte> bytes, std::size_t maximum_pixels)*



**brief** Decode an encoded image to RGBA8888.

**bytes** Encoded file contents; PNG today.

**maximum_pixels** Reject an image with more pixels than this, before allocating for it. Zero disables the check.

**return** The decoded image, or a DecodedImage carrying the decoder's message.



**note** Always produces four channels, whatever the source had, because every path above this wants RGBA8888 and converting once here is cheaper than every caller handling three formats.



**note** The dimension check happens before the decode allocates. A malicious or corrupt PNG header can claim 65535x65535, which is 17 GB of RGBA; finding that out after the allocation is too late on a phone.



**note** Internal to the squared build. Not installed, not public API: there is one consumer today, and the extension policy says a second concrete use comes before a public interface.

### decode_utf8

*std::optional<char32_t> decode_utf8(std::string_view text, std::size_t & cursor)*

*Defined at [graphics/src/detail/bitmap_font_detail.cppm:156](../../../../../graphics/src/detail/bitmap_font_detail.cppm#L156)*

### fail

*void fail(BitmapFontError & error, BitmapFontErrorCode code, std::size_t line, const char * message, const char * field)*

*Defined at [graphics/src/detail/bitmap_font_detail.cppm:29](../../../../../graphics/src/detail/bitmap_font_detail.cppm#L29)*

 Record a failure. `message` and `field` must be string literals: the error keeps the pointers, not copies.

### integer_field

*bool integer_field(const Fields & fields, const char * name, Integer & result, BitmapFontError & error, std::size_t line)*

*Defined at [graphics/src/detail/bitmap_font_detail.cppm:115](../../../../../graphics/src/detail/bitmap_font_detail.cppm#L115)*

### is_space

*bool is_space(char value)*

*Defined at [graphics/src/detail/bitmap_font_detail.cppm:17](../../../../../graphics/src/detail/bitmap_font_detail.cppm#L17)*

### parse_record

*bool parse_record(std::string_view line, std::string_view & tag, Fields & fields, BitmapFontError & error, std::size_t line_number)*

*Defined at [graphics/src/detail/bitmap_font_detail.cppm:39](../../../../../graphics/src/detail/bitmap_font_detail.cppm#L39)*

### safe_page_path

*bool safe_page_path(std::string_view path)*

*Defined at [graphics/src/detail/bitmap_font_detail.cppm:134](../../../../../graphics/src/detail/bitmap_font_detail.cppm#L134)*

### valid_scalar

*bool valid_scalar(char32_t value)*

*Defined at [graphics/src/detail/bitmap_font_detail.cppm:22](../../../../../graphics/src/detail/bitmap_font_detail.cppm#L22)*

