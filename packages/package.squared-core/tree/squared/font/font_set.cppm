export module sq.font:font_set;

export import :face;
import std;

export namespace sq::font
{

/**
 * @brief The faces a terminal draws with: a primary face (its cell size is
 * the grid's), its bold face if there is one, and fallback faces for what
 * the primary lacks (CJK, symbols), tried in order.
 *
 * Up to `max_faces` regular faces, one bold; all at one pixel size. Holds
 * the faces themselves (32 bytes each), so 320 bytes and FreeType's memory
 * for each face opened.
 *
 * @code
 * sq::font::FontSet fonts;
 * (void)fonts.add( std::move( jetbrains_mono ) );        // the primary
 * (void)fonts.set_bold( std::move( jetbrains_mono_bold ) );
 * fonts.add_system_fallbacks();                          // Noto CJK and the rest
 * (void)fonts.set_pixel_size( 32 );
 * @endcode
 */
class FontSet final
{
    public:
        static constexpr std::size_t max_faces = 8;

        /** Which face draws a character, and whether it is a real bold one. */
        struct Choice
        {
                std::uint8_t face = 0;
                bool bold_face = false;
        };

        /** A regular face, after those added before; the first is the primary. */
        [[nodiscard]] FaceError add( Face face );
        /** The primary's bold face. */
        [[nodiscard]] FaceError set_bold( Face face );

        /**
         * The system's fallback fonts that exist here, in order (Android's
         * Noto CJK, or the 5.1 tablet's Droid Sans Fallback, then symbols;
         * on Linux, Noto CJK and DejaVu): how many were added.
         */
        std::size_t add_system_fallbacks();

        /** Every face at `pixels`; the metrics are the primary's. */
        [[nodiscard]] FaceError set_pixel_size( std::uint16_t pixels );

        [[nodiscard]] FontMetrics metrics() const noexcept { return metrics_; }
        /** Regular faces held, the primary among them. */
        [[nodiscard]] std::size_t size() const noexcept { return count_; }
        [[nodiscard]] bool has_bold() const noexcept { return bold_.is_open(); }

        /** The face for `code`: for bold, the bold face when it has it; else the first that does. */
        [[nodiscard]] std::optional<Choice> choose( char32_t code, bool bold ) const noexcept;

        /**
         * `code` rendered by the face `choose` picks, `style` made where
         * that face lacks it. Nothing when no face has it. Valid until the
         * next render by the same face.
         */
        [[nodiscard]] std::optional<GlyphBitmap> render( char32_t code, GlyphStyle style,
                                                         Choice *chosen = nullptr ) noexcept;

        /** Paths `add_system_fallbacks` looks for, in order. */
        [[nodiscard]] static std::span<const std::string_view> system_fallback_paths() noexcept;

    private:
        std::array<Face, max_faces> faces_{};
        Face bold_{};
        FontMetrics metrics_{};
        std::uint16_t pixels_ = 0;
        std::uint8_t count_ = 0;
};

} // namespace sq::font
