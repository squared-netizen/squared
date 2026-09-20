// AtlasRegion accessors.
//
// Pure readers over metadata the containing TextureAtlas filled in. They are
// out of line rather than inline in the header so that atlas_region.hpp stays
// a declaration - every consumer of a TextureAtlas includes it, and it sits
// underneath the whole skin system.

#include <squared/graphics2d/atlas_region.hpp>

#include <squared/graphics2d/texture_region.hpp>

#include <string>

namespace sq::graphics2d {

const std::string& AtlasRegion::name() const noexcept
{
    return name_;
}

int AtlasRegion::index() const noexcept
{
    return index_;
}

int AtlasRegion::packed_width() const noexcept
{
    return packed_width_;
}

int AtlasRegion::packed_height() const noexcept
{
    return packed_height_;
}

bool AtlasRegion::rotated_clockwise() const noexcept
{
    // Delegated rather than duplicated. The TextureRegion already carries the
    // rotation because its UV arithmetic depends on it, and a second copy here
    // could disagree with the coordinates it is meant to describe.
    return region_.rotated_clockwise();
}

int AtlasRegion::original_width() const noexcept
{
    return original_width_;
}

int AtlasRegion::original_height() const noexcept
{
    return original_height_;
}

int AtlasRegion::offset_x() const noexcept
{
    return offset_x_;
}

int AtlasRegion::offset_y() const noexcept
{
    return offset_y_;
}

}  // namespace sq::graphics2d
