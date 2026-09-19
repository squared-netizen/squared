#pragma once

#include <squared/graphics2d/texture_region.hpp>

#include <array>
#include <optional>
#include <string>

namespace sq::graphics2d {

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

} // namespace sq::graphics2d
