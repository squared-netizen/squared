#include <squared_gui_showcase/showcase.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace {

class RecordingPainter final : public squared::gui::Painter {
public:
    squared::gui::Size measure_text(std::string_view text) override
    {
        return {static_cast<float>(text.size()) * 8.0F, 18.0F};
    }

    void fill_rectangle(
        const squared::gui::Rectangle& rectangle,
        squared::graphics::Color
    ) override
    {
        fills.push_back(rectangle);
    }

    void stroke_rectangle(
        const squared::gui::Rectangle&,
        squared::graphics::Color,
        float
    ) override
    {
    }

    void draw_region(
        const squared::graphics2d::TextureRegion&,
        const squared::gui::Rectangle&,
        squared::graphics::Color
    ) override
    {
        ++regions;
    }

    void draw_text(
        std::string_view,
        float,
        float,
        squared::graphics::Color
    ) override
    {
        ++texts;
    }

    void push_clip(const squared::gui::Rectangle&) override { ++clip_depth; }
    void pop_clip() override { --clip_depth; }

    std::vector<squared::gui::Rectangle> fills;
    int regions{0};
    int texts{0};
    int clip_depth{0};
};

void require(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
}

} // namespace

int main()
{
    auto image = std::make_shared<squared::gui::ColorDrawable>(
        squared::graphics::Color::from_rgba8(79, 137, 255),
        squared::gui::Size{96.0F, 32.0F}
    );
    squared_gui_showcase::Showcase showcase(
        960.0F, 540.0F, squared::gui::Skin{}, std::move(image)
    );
    RecordingPainter painter;
    showcase.ui().layout(painter);
    showcase.ui().paint(painter);

    require(showcase.ui().window_count() == 3,
            "showcase opens three independent windows");
    require(showcase.ui().content() != nullptr,
            "showcase installs a launcher widget tree");
    require(!painter.fills.empty() && painter.texts > 20,
            "showcase paints its panels, controls, and labels");
    require(painter.clip_depth == 0,
            "showcase balances nested painter clips");

    showcase.show_confirmation_dialog();
    showcase.ui().layout(painter);
    require(showcase.ui().window_count() == 4,
            "showcase can launch a modal confirmation dialog");
    require(showcase.ui().key_down(squared::gui::Key::escape),
            "showcase modal supports escape dismissal");
    require(showcase.ui().window_count() == 3,
            "dismissed modal is removed safely");

    showcase.show_about_dialog();
    showcase.ui().layout(painter);
    showcase.ui().paint(painter);
    require(showcase.ui().window_count() == 4,
            "showcase can launch a second dialog design");
    require(painter.clip_depth == 0,
            "dialog rendering keeps painter clips balanced");

    showcase.report_graphics_recovery(2, false);
    require(showcase.status().find("generation 2 rebuilt") !=
                std::string_view::npos,
            "showcase reports graphics-context reconstruction visibly");

    std::cout << "Squared GUI complete showcase: OK\n";
}
