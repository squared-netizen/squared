module sq.font;

import std;

namespace sq::font
{

namespace
{

std::uint8_t style_bits( GlyphStyle style ) noexcept
{
    return static_cast<std::uint8_t>( ( style.bold ? 1U : 0U ) | ( style.italic ? 2U : 0U ) );
}

constexpr std::uint8_t empty = 0;
constexpr std::uint8_t present = 1;
constexpr std::uint8_t missing = 2;

} // namespace

GlyphAtlas::GlyphAtlas( std::uint16_t width, std::uint16_t height, std::uint16_t capacity )
    : width_( std::max<std::uint16_t>( width, 1 ) )
    , height_( std::max<std::uint16_t>( height, 1 ) )
{
    capacity_ = std::max<std::size_t>( capacity, 1 );
    // Twice the glyphs in slots, a power of two: short probes.
    const std::size_t slots = std::bit_ceil( capacity_ * 2 );
    mask_ = slots - 1;
    slots_ = std::make_unique<Slot[]>( slots );
    pixels_ = std::make_unique<std::uint8_t[]>( std::size_t{ width_ } * height_ );
    dirty_ = { 0, 0, width_, height_ };
}

GlyphAtlas::Slot &GlyphAtlas::slot_for( char32_t code, std::uint8_t style ) noexcept
{
    std::size_t at = ( ( static_cast<std::size_t>( code ) * 2654435761U ) ^ style ) & mask_;
    for( ;; ) {
        Slot &slot = slots_[at];
        if( slot.state == empty || ( slot.code == code && slot.style == style ) ) {
            return slot;
        }
        at = ( at + 1 ) & mask_;
    }
}

void GlyphAtlas::clear() noexcept
{
    std::fill_n( slots_.get(), mask_ + 1, Slot{} );
    std::fill_n( pixels_.get(), std::size_t{ width_ } * height_, std::uint8_t{ 0 } );
    count_ = 0;
    pen_x_ = 0;
    shelf_y_ = 0;
    shelf_height_ = 0;
    dirty_ = { 0, 0, width_, height_ };
    ++generation_;
}

bool GlyphAtlas::place( const GlyphBitmap &bitmap, AtlasGlyph &glyph ) noexcept
{
    const std::uint16_t width = bitmap.width;
    const std::uint16_t height = bitmap.height;
    if( width == 0 || height == 0 ) {
        glyph.x = 0;
        glyph.y = 0;
        glyph.width = 0;
        glyph.height = 0;
        return true;
    }
    // A pixel between glyphs, so filtering never mixes two.
    if( width + 1U > width_ ) {
        return false;
    }
    if( pen_x_ + width + 1U > width_ ) {
        shelf_y_ = static_cast<std::uint16_t>( shelf_y_ + shelf_height_ );
        pen_x_ = 0;
        shelf_height_ = 0;
    }
    if( shelf_y_ + height + 1U > height_ ) {
        return false;
    }
    glyph.x = pen_x_;
    glyph.y = shelf_y_;
    glyph.width = width;
    glyph.height = height;
    for( std::uint16_t row = 0; row < height; ++row ) {
        const std::span<const std::uint8_t> from = bitmap.row( row );
        std::copy( from.begin(), from.end(),
                   &pixels_[( std::size_t{ glyph.y } + row ) * width_ + glyph.x] );
    }
    pen_x_ = static_cast<std::uint16_t>( pen_x_ + width + 1U );
    shelf_height_ =
        std::max<std::uint16_t>( shelf_height_, static_cast<std::uint16_t>( height + 1U ) );
    mark( glyph );
    return true;
}

void GlyphAtlas::mark( const AtlasGlyph &glyph ) noexcept
{
    if( dirty_.empty() ) {
        dirty_ = { glyph.x, glyph.y, glyph.width, glyph.height };
        return;
    }
    const int left = std::min<int>( dirty_.x, glyph.x );
    const int top = std::min<int>( dirty_.y, glyph.y );
    const int right = std::max<int>( dirty_.x + dirty_.width, glyph.x + glyph.width );
    const int bottom = std::max<int>( dirty_.y + dirty_.height, glyph.y + glyph.height );
    dirty_ = { static_cast<std::uint16_t>( left ), static_cast<std::uint16_t>( top ),
               static_cast<std::uint16_t>( right - left ),
               static_cast<std::uint16_t>( bottom - top ) };
}

std::optional<AtlasGlyph> GlyphAtlas::glyph( FontSet &fonts, char32_t code,
                                             GlyphStyle style ) noexcept
{
    const std::uint8_t bits = style_bits( style );
    Slot *slot = &slot_for( code, bits );
    if( slot->state == present ) {
        return slot->glyph;
    }
    if( slot->state == missing ) {
        return std::nullopt;
    }
    if( count_ >= capacity_ ) {
        clear();
        slot = &slot_for( code, bits );
    }
    FontSet::Choice chosen{};
    const std::optional<GlyphBitmap> bitmap = fonts.render( code, style, &chosen );
    slot->code = code;
    slot->style = bits;
    ++count_;
    if( !bitmap ) {
        slot->state = missing;
        return std::nullopt;
    }
    AtlasGlyph glyph{};
    glyph.left = bitmap->left;
    glyph.top = bitmap->top;
    glyph.advance = bitmap->advance;
    glyph.face = chosen.face;
    if( !place( *bitmap, glyph ) ) {
        // Full: start again, and place it first.
        clear();
        slot = &slot_for( code, bits );
        slot->code = code;
        slot->style = bits;
        ++count_;
        if( !place( *bitmap, glyph ) ) {
            slot->state = missing; // larger than the whole atlas
            return std::nullopt;
        }
    }
    slot->state = present;
    slot->glyph = glyph;
    return glyph;
}

} // namespace sq::font
