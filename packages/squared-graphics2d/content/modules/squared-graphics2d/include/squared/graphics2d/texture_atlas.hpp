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
    /**
     * @brief Read the logical region name.
     * @return Reference to the region's name; stable while the atlas lives.
     */
    [[nodiscard]] const std::string& name() const noexcept;

    /**
     * @brief Read the duplicate-region index.
     * @return Index for repeated names, or -1 when unindexed.
     */
    [[nodiscard]] int index() const noexcept;

    /**
     * @brief Access the drawable texture view.
     * @return Reference to the region's texture view.
     */
    [[nodiscard]] const TextureRegion& region() const noexcept
    {
        return region_;
    }

    /**
     * @brief Read the trimmed logical width before atlas rotation.
     * @return Width in source pixels.
     */
    [[nodiscard]] int packed_width() const noexcept;

    /**
     * @brief Read the trimmed logical height before atlas rotation.
     * @return Height in source pixels.
     */
    [[nodiscard]] int packed_height() const noexcept;

    /**
     * @brief Read the storage rotation flag.
     * @return true when storage is rotated 90 degrees clockwise.
     */
    [[nodiscard]] bool rotated_clockwise() const noexcept;

    /**
     * @brief Read the width before whitespace trimming.
     * @return Original width in source pixels.
     */
    [[nodiscard]] int original_width() const noexcept;

    /**
     * @brief Read the height before whitespace trimming.
     * @return Original height in source pixels.
     */
    [[nodiscard]] int original_height() const noexcept;

    /**
     * @brief Read the trimmed region's horizontal placement.
     * @return Horizontal offset in source pixels.
     */
    [[nodiscard]] int offset_x() const noexcept;

    /**
     * @brief Read the trimmed region's vertical placement.
     * @return Vertical offset in source pixels.
     */
    [[nodiscard]] int offset_y() const noexcept;

    /**
     * @brief Read optional nine-patch splits.
     * @return Left, right, top, bottom splits when present, else empty.
     */
    [[nodiscard]] const std::optional<std::array<int, 4>>&
    splits() const noexcept
    {
        return splits_;
    }

    /**
     * @brief Read optional nine-patch padding.
     * @return Left, right, top, bottom padding when present, else empty.
     */
    [[nodiscard]] const std::optional<std::array<int, 4>>&
    pads() const noexcept
    {
        return pads_;
    }

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
    /** @brief Construct an empty atlas owning no pages or regions. */
    TextureAtlas() noexcept = default;
    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;
    TextureAtlas(TextureAtlas&&) = delete;
    TextureAtlas& operator=(TextureAtlas&&) = delete;

    /** @brief Destroy every owned page texture and region. */
    ~TextureAtlas();

    /**
     * @brief Load an atlas asset without disturbing a valid prior load on
     * failure.
     * @param atlas_path Null-terminated path of the libGDX text atlas asset.
     * Page image paths are resolved relative to it.
     * @return true when every page and region loaded successfully.
     */
    [[nodiscard]] bool load(const char* atlas_path) noexcept;

    /**
     * @brief Load an atlas using one recovery policy for every page.
     *
     * ReloadFromAsset, RetainPixels, and Discard are supported. Regenerate is
     * rejected because atlas pages require distinct application recipes.
     * @param atlas_path Null-terminated path of the libGDX text atlas asset.
     * @param page_recovery Policy applied to every loaded page.
     * @return true when every page and region loaded successfully.
     */
    [[nodiscard]] bool load(
        const char* atlas_path,
        TextureRecoveryPolicy page_recovery
    ) noexcept;

    /** @brief Destroy every atlas page texture and region. */
    void destroy() noexcept;

    /**
     * @brief Release page GPU objects while preserving regions and load recipes.
     */
    void release() noexcept;

    /**
     * @brief Mark page GPU objects stale without issuing graphics calls.
     */
    void invalidate() noexcept;

    /**
     * @brief Restore every released page texture without invalidating regions.
     * @param context_preserved Whether the GPU context survived loss; true
     * skips recomputing fast-path state where possible.
     * @return true when every page texture was restored.
     */
    [[nodiscard]] bool restore(bool context_preserved = false) noexcept;

    /**
     * @brief Check whether any page and region is loaded.
     * @return true when the atlas owns at least one page and region.
     */
    [[nodiscard]] bool valid() const noexcept;

    /**
     * @brief Count loaded page textures.
     * @return Number of loaded page textures.
     */
    [[nodiscard]] std::size_t page_count() const noexcept;

    /**
     * @brief Count loaded regions.
     * @return Number of loaded regions.
     */
    [[nodiscard]] std::size_t region_count() const noexcept;

    /**
     * @brief Find an exact name/index pair, or return null.
     * @param name Region name to look up.
     * @param index Duplicate-region index, or -1 for the default entry.
     * @return Pointer to the matching region, or null when absent.
     */
    [[nodiscard]] const AtlasRegion* find_region(
        const std::string& name,
        int index = -1
    ) const noexcept;

private:
    std::vector<std::unique_ptr<Texture>> textures_;
    std::vector<AtlasRegion> regions_;
};

}  // namespace squared::graphics2d
