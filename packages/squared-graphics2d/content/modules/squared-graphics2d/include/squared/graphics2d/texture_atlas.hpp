#pragma once

#include <squared/graphics2d/texture.hpp>
#include <squared/graphics2d/texture_region.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace squared::graphics2d {

/**
 * @brief One named region and its libGDX-compatible atlas metadata.
 *
 * The TextureRegion and its Texture remain owned by the containing
 * TextureAtlas. AtlasRegion pointers become invalid when that atlas is
 * destroyed or successfully reloaded.
 */
class AtlasRegion final {
public:
    /** @brief Return the logical region name. */
    [[nodiscard]] const std::string& name() const noexcept;

    /** @brief Return the duplicate-region index, or -1 when unindexed. */
    [[nodiscard]] int index() const noexcept;

    /** @brief Return the drawable texture view. */
    [[nodiscard]] const TextureRegion& region() const noexcept;

    /** @brief Return the trimmed logical width before atlas rotation. */
    [[nodiscard]] int packed_width() const noexcept;

    /** @brief Return the trimmed logical height before atlas rotation. */
    [[nodiscard]] int packed_height() const noexcept;

    /** @brief Return whether storage is rotated 90 degrees clockwise. */
    [[nodiscard]] bool rotated_clockwise() const noexcept;

    /** @brief Return the width before whitespace trimming. */
    [[nodiscard]] int original_width() const noexcept;

    /** @brief Return the height before whitespace trimming. */
    [[nodiscard]] int original_height() const noexcept;

    /** @brief Return the trimmed region's horizontal placement. */
    [[nodiscard]] int offset_x() const noexcept;

    /** @brief Return the trimmed region's vertical placement. */
    [[nodiscard]] int offset_y() const noexcept;

    /** @brief Return optional left, right, top, and bottom nine-patch splits. */
    [[nodiscard]] const std::optional<std::array<int, 4>>&
    splits() const noexcept;

    /** @brief Return optional left, right, top, and bottom nine-patch padding. */
    [[nodiscard]] const std::optional<std::array<int, 4>>&
    pads() const noexcept;

private:
    friend class TextureAtlas;

    std::string name_;
    int index_{-1};
    TextureRegion region_;
    int packed_width_{0};
    int packed_height_{0};
    int original_width_{0};
    int original_height_{0};
    int offset_x_{0};
    int offset_y_{0};
    std::optional<std::array<int, 4>> splits_;
    std::optional<std::array<int, 4>> pads_;
};

/**
 * @brief Owning, transactionally loaded libGDX text texture atlas.
 *
 * Page image paths are resolved relative to the atlas asset. Multiple pages,
 * duplicate indexed names, rotation, trimming, filtering, repeat modes,
 * splits, and padding are supported. Packing source images is a separate
 * future asset-tools concern.
 */
class TextureAtlas final {
public:
    TextureAtlas() noexcept = default;
    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;
    TextureAtlas(TextureAtlas&&) = delete;
    TextureAtlas& operator=(TextureAtlas&&) = delete;
    ~TextureAtlas();

    /**
     * @brief Load an atlas asset without disturbing a valid prior load on
     * failure.
     */
    [[nodiscard]] bool load(const char* atlas_path) noexcept;

    /** @brief Destroy every atlas page texture and region. */
    void destroy() noexcept;

    /** @brief Return whether the atlas owns at least one page and region. */
    [[nodiscard]] bool valid() const noexcept;

    /** @brief Return the number of loaded page textures. */
    [[nodiscard]] std::size_t page_count() const noexcept;

    /** @brief Return the number of loaded regions. */
    [[nodiscard]] std::size_t region_count() const noexcept;

    /** @brief Find an exact name/index pair, or return null. */
    [[nodiscard]] const AtlasRegion* find_region(
        const std::string& name,
        int index = -1
    ) const noexcept;

private:
    std::vector<std::unique_ptr<Texture>> textures_;
    std::vector<AtlasRegion> regions_;
};

}  // namespace squared::graphics2d
