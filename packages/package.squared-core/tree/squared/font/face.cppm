export module sq.font:face;

export import :face_error;
export import :font_metrics;
export import :glyph_bitmap;
export import :glyph_style;
import std;

export namespace sq::font
{

/**
 * @brief One font face (a .ttf, .otf, or one face of a .ttc), at one pixel
 * size: its cell metrics, whether it has a character, and the character
 * rendered as 8-bit coverage.
 *
 * FreeType 2.15 underneath (third_party/freetype/), with its auto-hinter
 * in light mode (vertical hinting only: glyphs keep their shapes, stems
 * land on whole pixels) and anti-aliasing. Each face has its own FreeType
 * library, so faces on different threads never share state. A face is used
 * by one thread at a time.
 *
 * Memory: FreeType's own allocations, from the C heap: about 100 KB a face
 * opened from memory, more from a file (it keeps its own read buffer), and
 * the rendered glyph's buffer, reused. Rendering allocates only when a
 * glyph is larger than any before it.
 *
 * @code
 * sq::font::Face face;
 * if( const auto error = face.open_file( "fonts/JetBrainsMono-Regular.ttf" ) ) { report( error.what() ); }
 * face.set_pixel_size( 32 );
 * const auto cells = face.metrics();          // cell_width, cell_height, ascender ...
 * if( const auto glyph = face.render( U'A' ) ) { upload( *glyph ); }
 * @endcode
 */
class Face final
{
    public:
        Face() = default;
        Face( const Face & ) = delete;
        Face &operator=( const Face & ) = delete;
        Face( Face &&other ) noexcept;
        Face &operator=( Face &&other ) noexcept;
        ~Face();

        /** The face `index` of the font file at `path` (0 for a .ttf; a .ttc holds several). */
        [[nodiscard]] FaceError open_file( std::string_view path, int index = 0 );

        /**
         * The same from the file's bytes, which are not copied: they must
         * outlive the face (an asset mapped from the APK, say).
         */
        [[nodiscard]] FaceError open_memory( std::span<const std::byte> file, int index = 0 );

        /** Glyphs `pixels` high (the em size: about the height of a line); the metrics follow. */
        [[nodiscard]] FaceError set_pixel_size( std::uint16_t pixels );

        [[nodiscard]] bool is_open() const noexcept { return face_ != nullptr; }
        [[nodiscard]] FontMetrics metrics() const noexcept { return metrics_; }

        /** The face's family ("JetBrains Mono"), empty when none is open. */
        [[nodiscard]] std::string_view family() const noexcept;
        /** The face is a bold one of its family. */
        [[nodiscard]] bool bold() const noexcept;
        /** The face is an italic one of its family. */
        [[nodiscard]] bool italic() const noexcept;

        /** It has a glyph for `code` (else a fallback face draws it). */
        [[nodiscard]] bool has_glyph( char32_t code ) const noexcept;

        /**
         * `code` rendered, `style` applied where the face lacks it (bold on
         * a regular face is emboldened; italic is slanted). Nothing when the
         * face has no glyph for it or no size is set. Valid until the next
         * `render`.
         */
        [[nodiscard]] std::optional<GlyphBitmap> render( char32_t code,
                                                         GlyphStyle style = {} ) noexcept;

    private:
        void close() noexcept;
        [[nodiscard]] FaceError opened( int error, int index ) noexcept;

        // FreeType's FT_Library and FT_Face; their types stay out of the
        // interface (src/face.cpp).
        void *library_ = nullptr;
        void *face_ = nullptr;
        FontMetrics metrics_{};
        bool sized_ = false;
};

static_assert( sizeof( Face ) <= 32, "a Face grew: see docs/developer/font.md" );

} // namespace sq::font
