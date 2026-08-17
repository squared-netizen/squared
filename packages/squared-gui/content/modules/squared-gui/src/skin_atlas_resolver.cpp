#include <squared/gui/skin_loader.hpp>

#include <memory>
#include <optional>
#include <string>

namespace squared::gui {

DrawablePtr resolve_atlas_drawable(
    const graphics2d::TextureAtlas& atlas,
    std::string_view resource_name
)
{
    const auto* region = atlas.find_region(std::string(resource_name));
    if (!region) return {};
    if (!region->splits()) {
        return std::make_shared<RegionDrawable>(region->region());
    }
    const auto& split = *region->splits();
    std::optional<Insets> insets;
    if (region->pads()) {
        const auto& pad = *region->pads();
        insets = Insets{
            static_cast<float>(pad[0]), static_cast<float>(pad[2]),
            static_cast<float>(pad[1]), static_cast<float>(pad[3])
        };
    }
    return std::make_shared<NinePatchDrawable>(
        region->region(),
        NinePatchSplits{split[0], split[2], split[1], split[3]},
        insets
    );
}

} // namespace squared::gui
