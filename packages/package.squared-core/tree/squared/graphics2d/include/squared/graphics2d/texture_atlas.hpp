#pragma once

#include <squared/files/file_handle.hpp>
#include <squared/graphics2d/atlas_region.hpp>
#include <squared/graphics2d/texture.hpp>
#include <squared/graphics2d/texture_recovery_policy.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace sq::graphics {
class Context;
}  // namespace sq::graphics

namespace sq::graphics2d {

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
    [[nodiscard]] bool load(const files::FileHandle& atlas) noexcept;

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
        const files::FileHandle& atlas,
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
    [[nodiscard]] bool restore(const graphics::Context& graphics) noexcept;

    /**
     * @brief Rebuild, stating explicitly whether the context survived.
     * @param context_preserved true when the GPU objects are still valid.
     *
     * @note No default. The previous one was `false`, which made `restore()`
     * read as cheap and behave destructively: it reloaded everything, moved
     * every texture's generation, and invalidated every TextureRegion copied
     * out of an atlas - on the first frame, before anything had been lost.
     *
     * @note Prefer the overload taking a Context. It reads the answer from
     * the object that knows it, so the polarity cannot be got backwards.
     */
    [[nodiscard]] bool restore(bool context_preserved) noexcept;

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

} // namespace sq::graphics2d
