export module sq.font:glyph_atlas;

export import :atlas_glyph;
export import :font_set;
import std;

export namespace sq::font
{

/**
 * @brief The glyphs a terminal has drawn, rendered once into one 8-bit
 * coverage image, for one texture: a glyph is rendered the first time it is
 * asked for and found after that.
 *
 * Packed in shelves (rows of glyphs as tall as the tallest of the row), a
 * pixel apart so a texture's filtering never bleeds. When it is full, or
 * holds `capacity` glyphs, it starts again empty and `generation()` moves
 * on: an owner keeping atlas places drops them. A glyph no face has is
 * remembered too, so it is not looked for each frame.
 *
 * Memory, at construction only: width x height bytes (1 MB for the default
 * 1024 x 1024: about 1,200 cells of 19 x 43, JetBrains Mono at 32 px), and
 * 24 bytes a table slot (two slots a glyph: 96 KB for 2,048 glyphs).
 * `glyph` never allocates.
 *
 * @code
 * sq::font::GlyphAtlas atlas;                         // 1024 x 1024
 * if( const auto g = atlas.glyph( fonts, U'A', {} ) ) { quad( g->x, g->y, g->width, g->height ); }
 * if( !atlas.dirty().empty() ) { upload( atlas.pixels(), atlas.dirty() ); atlas.clear_dirty(); }
 * @endcode
 */
class GlyphAtlas final
{
    public:
        /** A rectangle of the image that changed since `clear_dirty`. */
        struct Rect
        {
                std::uint16_t x = 0;
                std::uint16_t y = 0;
                std::uint16_t width = 0;
                std::uint16_t height = 0;

                [[nodiscard]] bool empty() const noexcept { return width == 0 || height == 0; }
                friend constexpr bool operator==( const Rect &, const Rect & ) noexcept = default;
        };

        explicit GlyphAtlas( std::uint16_t width = 1024, std::uint16_t height = 1024,
                             std::uint16_t capacity = 2048 );

        /** Where `code` in `style` is, rendered by `fonts` now if it was not yet; nothing when no face has it. */
        [[nodiscard]] std::optional<AtlasGlyph> glyph( FontSet &fonts, char32_t code,
                                                       GlyphStyle style ) noexcept;

        /** Every glyph forgotten (after the fonts' size changed); `generation()` moves on. */
        void clear() noexcept;

        [[nodiscard]] std::span<const std::uint8_t> pixels() const noexcept
        {
            return { pixels_.get(), std::size_t{ width_ } * height_ };
        }
        [[nodiscard]] std::uint16_t width() const noexcept { return width_; }
        [[nodiscard]] std::uint16_t height() const noexcept { return height_; }
        [[nodiscard]] Rect dirty() const noexcept { return dirty_; }
        void clear_dirty() noexcept { dirty_ = {}; }
        [[nodiscard]] std::uint32_t generation() const noexcept { return generation_; }
        /** Glyphs held, those no face has among them. */
        [[nodiscard]] std::size_t size() const noexcept { return count_; }

    private:
        struct Slot
        {
                char32_t code = 0;
                AtlasGlyph glyph{};
                std::uint8_t style = 0;
                /** 0 empty, 1 a glyph, 2 no face has it. */
                std::uint8_t state = 0;
        };

        [[nodiscard]] Slot &slot_for( char32_t code, std::uint8_t style ) noexcept;
        [[nodiscard]] bool place( const GlyphBitmap &bitmap, AtlasGlyph &glyph ) noexcept;
        void mark( const AtlasGlyph &glyph ) noexcept;

        std::unique_ptr<std::uint8_t[]> pixels_;
        std::unique_ptr<Slot[]> slots_;
        std::size_t mask_ = 0;
        std::size_t count_ = 0;
        std::size_t capacity_ = 0;
        Rect dirty_{};
        std::uint32_t generation_ = 0;
        std::uint16_t width_ = 0;
        std::uint16_t height_ = 0;
        std::uint16_t pen_x_ = 0;
        std::uint16_t shelf_y_ = 0;
        std::uint16_t shelf_height_ = 0;
};

} // namespace sq::font
