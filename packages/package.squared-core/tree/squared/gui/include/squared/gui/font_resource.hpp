#pragma once

#include <squared/graphics2d/bitmap_font.hpp>
#include <squared/graphics2d/texture_region.hpp>

#include <optional>
#include <span>
#include <string>
#include <vector>

namespace sq::graphics2d {
struct GlyphPlacement;
} // namespace sq::graphics2d

namespace sq::gui {

/**
 * @brief Immutable portable bitmap-font resource selected by GUI styles.
 *
 * A descriptor-only resource records the safe asset path imported from a
 * skin. A resolved resource additionally owns parsed BMFont metrics and
 * non-owning texture-page regions. The textures referenced by those regions
 * must outlive the resource. FontResource performs no I/O and has no HoloDisk
 * or rendering-backend dependency.
 */
class FontResource final {
public:
    /**
     * @brief Construct a descriptor-only resource.
     * @param descriptor_path Contained relative BMFont descriptor path.
     * @throws std::invalid_argument when the path is empty, absolute, contains
     * a backslash, or contains an empty, `.` or `..` component.
     */
    explicit FontResource(std::string descriptor_path);

    /**
     * @brief Construct a resolved bitmap-font resource.
     * @param descriptor_path Contained relative descriptor identity.
     * @param font Valid parsed BMFont value transferred into the resource.
     * @param pages Page regions in BMFont page-id order. Each region must be
     * at least as large as the descriptor page dimensions and its texture
     * must outlive this resource.
     * @param scale Uniform logical-unit scale; finite and greater than zero.
     * @throws std::invalid_argument when any invariant is not satisfied.
     */
    FontResource(
        std::string descriptor_path,
        graphics2d::BitmapFont font,
        std::vector<graphics2d::TextureRegion> pages,
        float scale = 1.0F
    );

    /** @brief Return the descriptor asset path owned by this resource. */
    [[nodiscard]] const std::string& descriptor_path() const noexcept
    {
        return descriptor_path_;
    }

    /** @brief Return true when parsed metrics and matching pages are present. */
    [[nodiscard]] bool resolved() const noexcept { return font_.has_value(); }

    /** @brief Return parsed metrics, or null for a descriptor-only resource. */
    [[nodiscard]] const graphics2d::BitmapFont* bitmap_font() const noexcept
    {
        return font_ ? &*font_ : nullptr;
    }

    /** @brief Return page regions in BMFont page-id order. */
    [[nodiscard]] std::span<const graphics2d::TextureRegion> pages() const noexcept
    {
        return pages_;
    }

    /**
     * @brief Derive the texture view for one placement from this font.
     * @param glyph Placement produced from this resource's BitmapFont.
     * @return Non-owning page subregion, or an empty region when unresolved
     * or when the placement does not fit its declared page.
     */
    [[nodiscard]] graphics2d::TextureRegion glyph_region(
        const graphics2d::GlyphPlacement& glyph
    ) const noexcept;

    /** @brief Return the uniform conversion from source pixels to GUI units. */
    [[nodiscard]] float scale() const noexcept { return scale_; }

private:
    std::string descriptor_path_;
    std::optional<graphics2d::BitmapFont> font_;
    std::vector<graphics2d::TextureRegion> pages_;
    float scale_{1.0F};
};

} // namespace sq::gui
