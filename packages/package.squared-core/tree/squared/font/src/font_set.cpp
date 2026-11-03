module sq.font;

import std;

namespace sq::font
{

namespace
{

// Android's fallback fonts for what a terminal font lacks: CJK (Noto Sans
// CJK since Android 7; Droid Sans Fallback before it, the 5.1 tablet's),
// then symbols. Then a Linux desktop's (Debian's paths), so the same code
// finds fallbacks on the host.
constexpr std::array<std::string_view, 9> fallback_paths{
    "/system/fonts/NotoSansCJK-Regular.ttc",
    "/system/fonts/NotoSansSC-Regular.otf",
    "/system/fonts/DroidSansFallbackFull.ttf",
    "/system/fonts/DroidSansFallback.ttf",
    "/system/fonts/NotoSansSymbols-Regular-Subsetted.ttf",
    "/system/fonts/NotoSansSymbols-Regular-Subsetted2.ttf",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
};

} // namespace

std::span<const std::string_view> FontSet::system_fallback_paths() noexcept
{
    return fallback_paths;
}

FaceError FontSet::add( Face face )
{
    if( !face.is_open() ) {
        return { FaceErrorCode::not_open, 0 };
    }
    if( count_ == max_faces ) {
        return { FaceErrorCode::set_full, 0 };
    }
    if( pixels_ != 0 ) {
        if( const FaceError error = face.set_pixel_size( pixels_ ) ) {
            return error;
        }
    }
    faces_[count_] = std::move( face );
    ++count_;
    if( count_ == 1 ) {
        metrics_ = faces_[0].metrics();
    }
    return {};
}

FaceError FontSet::set_bold( Face face )
{
    if( !face.is_open() ) {
        return { FaceErrorCode::not_open, 0 };
    }
    if( pixels_ != 0 ) {
        if( const FaceError error = face.set_pixel_size( pixels_ ) ) {
            return error;
        }
    }
    bold_ = std::move( face );
    return {};
}

std::size_t FontSet::add_system_fallbacks()
{
    std::size_t added = 0;
    for( const std::string_view path : fallback_paths ) {
        if( count_ == max_faces ) {
            break;
        }
        std::error_code error;
        if( !std::filesystem::is_regular_file( std::filesystem::path{ path }, error ) ) {
            continue;
        }
        Face face;
        if( face.open_file( path ) ) {
            continue; // not a font FreeType reads: skipped
        }
        if( !add( std::move( face ) ) ) {
            ++added;
        }
    }
    return added;
}

FaceError FontSet::set_pixel_size( std::uint16_t pixels )
{
    if( count_ == 0 ) {
        return { FaceErrorCode::not_open, 0 };
    }
    for( std::uint8_t i = 0; i < count_; ++i ) {
        if( const FaceError error = faces_[i].set_pixel_size( pixels ) ) {
            return error;
        }
    }
    if( bold_.is_open() ) {
        if( const FaceError error = bold_.set_pixel_size( pixels ) ) {
            return error;
        }
    }
    pixels_ = pixels;
    metrics_ = faces_[0].metrics();
    return {};
}

std::optional<FontSet::Choice> FontSet::choose( char32_t code, bool bold ) const noexcept
{
    if( bold && bold_.has_glyph( code ) ) {
        return Choice{ .face = 0, .bold_face = true };
    }
    for( std::uint8_t i = 0; i < count_; ++i ) {
        if( faces_[i].has_glyph( code ) ) {
            return Choice{ .face = i, .bold_face = false };
        }
    }
    return std::nullopt;
}

std::optional<GlyphBitmap> FontSet::render( char32_t code, GlyphStyle style,
                                            Choice *chosen ) noexcept
{
    const std::optional<Choice> choice = choose( code, style.bold );
    if( !choice ) {
        return std::nullopt;
    }
    if( chosen != nullptr ) {
        *chosen = *choice;
    }
    Face &face = choice->bold_face ? bold_ : faces_[choice->face];
    return face.render( code, style );
}

} // namespace sq::font
