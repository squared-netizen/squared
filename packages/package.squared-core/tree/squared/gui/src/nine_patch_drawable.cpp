#include <squared/gui/nine_patch_drawable.hpp>

#include "detail/gui_detail.hpp"
#include <squared/graphics/color.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/gui/insets.hpp>
#include <squared/gui/nine_patch_splits.hpp>
#include <squared/gui/painter.hpp>
#include <squared/gui/rectangle.hpp>
#include <squared/gui/size.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>

namespace sq::gui {

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

NinePatchDrawable::NinePatchDrawable(
    const graphics2d::TextureRegion& region,
    NinePatchSplits splits,
    std::optional<Insets> content_insets
)
    : splits_(splits)
{
    if (splits.left < 0 || splits.top < 0 || splits.right < 0 ||
        splits.bottom < 0 || splits.left + splits.right > region.width() ||
        splits.top + splits.bottom > region.height()) {
        throw std::invalid_argument("Nine-patch splits exceed the source region");
    }
    const Insets requested_insets = content_insets.value_or(Insets{
        static_cast<float>(splits.left),
        static_cast<float>(splits.top),
        static_cast<float>(splits.right),
        static_cast<float>(splits.bottom)
    });
    insets_ = {
        std::max(0.0F, requested_insets.left),
        std::max(0.0F, requested_insets.top),
        std::max(0.0F, requested_insets.right),
        std::max(0.0F, requested_insets.bottom)
    };

    const int widths[]{
        splits.left,
        region.width() - splits.left - splits.right,
        splits.right
    };
    const int heights[]{
        splits.top,
        region.height() - splits.top - splits.bottom,
        splits.bottom
    };
    int source_y = 0;
    for (std::size_t row = 0; row < 3; ++row) {
        int source_x = 0;
        for (std::size_t column = 0; column < 3; ++column) {
            if (widths[column] > 0 && heights[row] > 0) {
                regions_[row * 3 + column] = region.subregion(
                    source_x, source_y, widths[column], heights[row]
                );
            }
            source_x += widths[column];
        }
        source_y += heights[row];
    }
}

Size NinePatchDrawable::minimum_size() const noexcept
{
    return {
        static_cast<float>(splits_.left + splits_.right),
        static_cast<float>(splits_.top + splits_.bottom)
    };
}

Insets NinePatchDrawable::content_insets() const noexcept { return insets_; }

void NinePatchDrawable::draw(
    Painter& painter,
    const Rectangle& rectangle,
    graphics::Color tint
) const
{
    const auto segments = [](float total, float leading, float trailing) {
        const float fixed = leading + trailing;
        if (fixed > 0.0F && total < fixed) {
            const float scale = std::max(0.0F, total) / fixed;
            leading *= scale;
            trailing *= scale;
        }
        return std::array<float, 3>{
            leading, std::max(0.0F, total - leading - trailing), trailing
        };
    };
    const auto widths = segments(
        rectangle.width,
        static_cast<float>(splits_.left),
        static_cast<float>(splits_.right)
    );
    const auto heights = segments(
        rectangle.height,
        static_cast<float>(splits_.top),
        static_cast<float>(splits_.bottom)
    );

    float destination_y = rectangle.y;
    for (std::size_t row = 0; row < 3; ++row) {
        float destination_x = rectangle.x;
        for (std::size_t column = 0; column < 3; ++column) {
            const auto& region = regions_[row * 3 + column];
            if (region.width() > 0 && region.height() > 0 &&
                widths[column] > 0.0F && heights[row] > 0.0F) {
                painter.draw_region(
                    region,
                    {destination_x, destination_y,
                     widths[column], heights[row]},
                    tint
                );
            }
            destination_x += widths[column];
        }
        destination_y += heights[row];
    }
}

} // namespace sq::gui
