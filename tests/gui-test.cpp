#include <squared/gui/gui.hpp>

#include <cstdlib>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {

using squared::gui::Rectangle;
using squared::gui::Size;

class RecordingPainter final : public squared::gui::Painter {
public:
    [[nodiscard]] Size measure_text(std::string_view text) override
    {
        return {static_cast<float>(text.size()) * 8.0F, 16.0F};
    }

    void fill_rectangle(
        const Rectangle& rectangle,
        squared::graphics::Color
    ) override
    {
        fills.push_back(rectangle);
    }

    void stroke_rectangle(
        const Rectangle&,
        squared::graphics::Color,
        float
    ) override
    {
        ++strokes;
    }

    void draw_region(
        const squared::graphics2d::TextureRegion&,
        const Rectangle& rectangle,
        squared::graphics::Color
    ) override
    {
        regions.push_back(rectangle);
    }

    void draw_text(
        std::string_view text,
        float,
        float,
        squared::graphics::Color
    ) override
    {
        strings.emplace_back(text);
    }

    void push_clip(const Rectangle&) override { ++clip_depth; }
    void pop_clip() override { --clip_depth; }

    std::vector<Rectangle> fills;
    std::vector<Rectangle> regions;
    std::vector<std::string> strings;
    int strokes{0};
    int clip_depth{0};
};

void require(bool condition, std::string_view message)
{
    if (!condition) {
        std::cerr << "GUI test failure: " << message << '\n';
        std::exit(1);
    }
}

std::string read_text(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    return {
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>()
    };
}

bool has_png_signature(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    std::array<unsigned char, 8> bytes{};
    stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    return stream.gcount() == static_cast<std::streamsize>(bytes.size()) &&
        bytes == std::array<unsigned char, 8>{
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
        };
}

} // namespace

int main()
{
    using namespace squared::gui;

    RecordingPainter painter;

    const std::filesystem::path skin_root{SQUARED_GUI_TEST_ASSET_DIR};
    const std::string atlas = read_text(skin_root / "skin.atlas");
    const std::string license = read_text(skin_root / "LICENSE.txt");
    for (const std::string_view region : {
             "button.normal", "button.pressed", "checkbox.unchecked",
             "checkbox.checked", "slider.track", "slider.knob"
         }) {
        require(atlas.find(region) != std::string::npos,
                "test atlas declares every skin region");
    }
    require(license.find("Creative Commons Zero") != std::string::npos,
            "test skin records its CC0 license");
    for (const std::string_view image : {
             "button-normal.png", "button-pressed.png",
             "checkbox-unchecked.png", "checkbox-checked.png",
             "slider-track.png", "slider-knob.png"
         }) {
        require(has_png_signature(skin_root / image),
                "test skin contains valid PNG assets");
    }

    Skin skin;
    auto test_normal = std::make_shared<ColorDrawable>(
        squared::graphics::Color::from_rgba8(30, 50, 80),
        Size{0.0F, 48.0F},
        Insets{14, 8, 14, 8}
    );
    auto test_pressed = std::make_shared<ColorDrawable>(
        squared::graphics::Color::from_rgba8(40, 100, 190),
        Size{0.0F, 48.0F}
    );
    skin.add_drawable("kenney.button", test_normal);
    skin.add_button_style("kenney", {
        test_normal,
        test_pressed,
        test_pressed,
        test_normal,
        skin.text,
        skin.muted_text,
        48.0F,
        14.0F
    });
    require(skin.drawable("kenney.button") == test_normal,
            "skin returns named drawable resources");

    Ui ui(320.0F, 240.0F, std::move(skin));
    auto column = std::make_unique<LinearLayout>(Direction::vertical);
    auto title = std::make_unique<Label>("Cartridge name");
    auto field = std::make_unique<TextField>("demo");
    auto check = std::make_unique<CheckBox>("Writable", false);
    auto slider = std::make_unique<Slider>(0.0F, 10.0F, 2.0F);
    auto button = std::make_unique<Button>("Mount");
    field->set_style("default");
    button->set_style("kenney");
    TextField* field_pointer = field.get();
    CheckBox* check_pointer = check.get();
    Slider* slider_pointer = slider.get();
    Button* button_pointer = button.get();
    int clicks = 0;
    button->set_on_click([&clicks] { ++clicks; });

    column->add(std::move(title));
    column->add(std::move(field));
    column->add(std::move(check));
    column->add(std::move(slider));
    column->add(std::move(button));
    ui.set_content(std::move(column));
    ui.layout(painter);

    require(field_pointer->width() == 304.0F, "vertical layout fills width");
    require(field_pointer->height() >= 44.0F, "touch controls honor minimum size");
    require(ui.content()->layout_valid(), "layout validation marks tree valid");
    field_pointer->set_text("changed");
    require(!ui.content()->layout_valid(), "child changes invalidate ancestors");
    ui.layout(painter);

    squared::application::Event event;
    event.type = squared::application::Event::Type::PointerDown;
    event.pointer_id = 7;
    event.x = 20.0F;
    event.y = field_pointer->y() + 10.0F;
    require(ui.event(event), "framework pointer event reaches text field");
    event.type = squared::application::Event::Type::PointerUp;
    ui.event(event);
    require(ui.focused() == field_pointer, "framework event assigns focus");
    require(ui.text_input(" disk"), "focused field accepts text");
    require(field_pointer->text() == "changed disk", "text insertion is retained");
    require(ui.key_down(Key::backspace), "field handles existing key path");
    require(field_pointer->text() == "changed dis", "backspace edits text");

    event.type = squared::application::Event::Type::PointerDown;
    event.y = check_pointer->y() + 10.0F;
    require(ui.event(event), "checkbox accepts framework event");
    event.type = squared::application::Event::Type::PointerUp;
    require(ui.event(event), "checkbox completes framework click");
    require(check_pointer->checked(), "checkbox toggles through existing routing");

    event.type = squared::application::Event::Type::PointerDown;
    event.x = 160.0F;
    event.y = slider_pointer->y() + 10.0F;
    require(ui.event(event), "slider captures framework pointer id");
    event.type = squared::application::Event::Type::PointerMove;
    event.x = 300.0F;
    ui.event(event);
    event.type = squared::application::Event::Type::PointerUp;
    ui.event(event);
    require(slider_pointer->value() > 8.0F, "slider tracks captured pointer");

    event.type = squared::application::Event::Type::PointerDown;
    event.x = 20.0F;
    event.y = button_pointer->y() + 10.0F;
    require(ui.event(event), "skinned button captures pointer");
    event.type = squared::application::Event::Type::PointerUp;
    require(ui.event(event), "skinned button releases pointer");
    require(clicks == 1, "button callback runs once");
    require(ui.focused() == button_pointer, "button receives focus");
    require(ui.key_down(Key::enter), "button retains existing key path");
    require(clicks == 2, "keyboard activation runs callback");

    auto stack = std::make_unique<Stack>();
    stack->set_size(100.0F, 80.0F);
    Widget& stacked = stack->add(std::make_unique<Panel>());
    stack->validate_layout(painter, ui.skin());
    require(stacked.width() == 100.0F && stacked.height() == 80.0F,
            "stack fills every child");

    ui.paint(painter);
    require(painter.clip_depth == 0, "paint clips are balanced");
    require(!painter.fills.empty(), "skin drawables use painter commands");
    require(painter.strings.size() >= 4, "basic controls emit text");

    squared::graphics2d::TextureRegion empty_region;
    RegionDrawable region_drawable(empty_region);
    region_drawable.draw(painter, {0, 0, 16, 16});
    require(painter.regions.size() == 1,
            "region skins draw through Graphics2D TextureRegion");

    std::cout << "Squared GUI skins, controls, layout, and framework events: OK\n";
    return 0;
}
