// Ui::set_safe_area: where the content sits.
//
// What a phone shows - a bar over the top of the screen - cannot be seen from
// a unit test, but the rectangle the content is given can be read back.

#include <squared/gui/linear_layout.hpp>
#include <squared/gui/src/insets.hpp>
#include <squared/gui/ui.hpp>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>

using namespace sq::gui;

namespace {

bool near(float a, float b) { return std::fabs(a - b) < 0.001F; }

}  // namespace

int main()
{
    Ui ui(720.0F, 1280.0F);
    Widget& content = ui.set_content(std::make_unique<LinearLayout>());

    // Nothing set: the content fills the viewport, as it always did.
    assert(near(content.x(), 0.0F) && near(content.y(), 0.0F));
    assert(near(content.width(), 720.0F) && near(content.height(), 1280.0F));

    // A status bar over the top and a navigation bar under the bottom.
    ui.set_safe_area({.left = 0.0F, .top = 48.0F, .right = 0.0F, .bottom = 32.0F});
    assert(near(content.y(), 48.0F));
    assert(near(content.height(), 1280.0F - 48.0F - 32.0F));
    assert(near(content.width(), 720.0F));

    // Resizing keeps the insets: rotation must not put the content back under
    // the bar.
    ui.resize(1280.0F, 720.0F);
    assert(near(content.y(), 48.0F));
    assert(near(content.width(), 1280.0F));
    assert(near(content.height(), 720.0F - 48.0F - 32.0F));

    // Replacing the content keeps them too: every screen change does this.
    Widget& next = ui.set_content(std::make_unique<LinearLayout>());
    assert(near(next.y(), 48.0F));
    assert(near(next.height(), 720.0F - 48.0F - 32.0F));

    // A cut-out on the side in landscape.
    ui.set_safe_area({.left = 60.0F, .top = 0.0F, .right = 0.0F, .bottom = 0.0F});
    assert(near(next.x(), 60.0F) && near(next.y(), 0.0F));
    assert(near(next.width(), 1280.0F - 60.0F));

    // Nonsense is clamped, never propagated.
    const float nan = std::numeric_limits<float>::quiet_NaN();
    ui.set_safe_area({.left = -5.0F, .top = nan, .right = 0.0F, .bottom = 0.0F});
    assert(near(ui.safe_area().left, 0.0F) && near(ui.safe_area().top, 0.0F));
    assert(near(next.x(), 0.0F) && near(next.y(), 0.0F));

    // Insets larger than the viewport leave an empty rectangle, not a negative one.
    ui.set_safe_area({.left = 2000.0F, .top = 2000.0F, .right = 0.0F, .bottom = 0.0F});
    assert(next.width() >= 0.0F && next.height() >= 0.0F);

    std::puts("ui safe area: all assertions passed");
    return 0;
}
