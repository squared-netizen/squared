#include <squared/gui/gui.hpp>
#include <squared/gui/skin_loader.hpp>

#include <cstdlib>
#include <algorithm>
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
        const squared::graphics2d::TextureRegion& region,
        const Rectangle& rectangle,
        squared::graphics::Color
    ) override
    {
        regions.push_back(rectangle);
        region_sizes.push_back({
            static_cast<float>(region.width()),
            static_cast<float>(region.height())
        });
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
    std::vector<Size> region_sizes;
    std::vector<std::string> strings;
    int strokes{0};
    int clip_depth{0};
};

class RecordingTextInput final
    : public squared::application::TextInputService {
public:
    void start(
        const squared::application::TextInputRequest& request
    ) override
    {
        last_area = request.area;
        active_state = true;
        ++starts;
    }
    void update_area(
        const squared::application::TextInputArea& area
    ) override
    {
        last_area = area;
        ++updates;
    }
    void stop() override { active_state = false; ++stops; }
    [[nodiscard]] bool active() const noexcept override { return active_state; }

    squared::application::TextInputArea last_area{};
    bool active_state{false};
    int starts{0};
    int updates{0};
    int stops{0};
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

void verify_pinned_gdx_skins()
{
#ifdef SQUARED_GDX_SKINS_ASSET_DIR
    const std::filesystem::path root{SQUARED_GDX_SKINS_ASSET_DIR};
    const std::string pin = read_text(root / "PIN.sha256");
    require(
        pin.starts_with(SQUARED_GDX_SKINS_EXPECTED_SHA),
        "gdx-skins asset matches its configured SHA-256 pin"
    );
    const std::string index = read_text(root / "ASSET_INDEX.txt");
    require(index.find(".json") != std::string::npos &&
                index.find(".atlas") != std::string::npos &&
                index.find(".png") != std::string::npos,
            "gdx-skins index records JSON, atlas, and PNG assets");
    require(index.find("gdx-holo/skin/uiskin.json") != std::string::npos,
            "the pinned archive contains the selected gdx-holo skin");
    std::ifstream archive(root / "gdx-skins.zip", std::ios::binary);
    std::array<char, 4> signature{};
    archive.read(signature.data(), signature.size());
    require(archive.gcount() == 4 && signature[0] == 'P' &&
                signature[1] == 'K',
            "gdx-skins remains an embedded ZIP archive");
    const auto selected = root / "selected" / "gdx-holo";
    require(has_png_signature(selected / "uiskin.png") &&
                !read_text(selected / "uiskin.atlas").empty() &&
                !read_text(selected / "default.fnt").empty(),
            "selected gdx-holo runtime assets are materialized");
    squared::gui::Skin holo;
    squared::gui::SkinLoadReport holo_report;
    require(
        squared::gui::load_libgdx_skin(
            holo,
            read_text(selected / "uiskin.json"),
            [](std::string_view) {
                return std::make_shared<squared::gui::ColorDrawable>(
                    squared::graphics::Color::white()
                );
            },
            holo_report
        ) && holo_report.styles_loaded >= 10,
        "the pinned gdx-holo JSON maps to supported Squared styles"
    );
#endif
}

} // namespace

int main()
{
    using namespace squared::gui;

    RecordingPainter painter;

    verify_pinned_gdx_skins();

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
    require(atlas.find("split: 16, 16, 16, 16") != std::string::npos &&
                atlas.find("pad: 12, 12, 10, 10") != std::string::npos,
            "test skin includes libGDX nine-patch metadata");
    for (const std::string_view image : {
             "button-normal.png", "button-pressed.png",
             "checkbox-unchecked.png", "checkbox-checked.png",
             "slider-track.png", "slider-knob.png"
         }) {
        require(has_png_signature(skin_root / image),
                "test skin contains valid PNG assets");
    }

    Skin skin;
    {
        const std::string_view relaxed_skin = R"skin(
        {
          com.badlogic.gdx.graphics.g2d.BitmapFont: {
            default-font: { file: default.fnt }
          },
          com.badlogic.gdx.graphics.Color: {
            ink: { r: 0.1, g: 0.2, b: 0.3, a: 1 },
            muted: { r: 0.5, g: 0.5, b: 0.5, a: 1 }
          },
          com.badlogic.gdx.scenes.scene2d.ui.TextButton$TextButtonStyle: {
            default: { up: button-up, down: button-down,
              disabled: button-disabled, fontColor: ink,
              disabledFontColor: muted, font: default-font }
          },
          com.badlogic.gdx.scenes.scene2d.ui.TextField$TextFieldStyle: {
            default: { background: field, focusedBackground: field-focus,
              fontColor: ink, font: default-font }
          },
          com.badlogic.gdx.scenes.scene2d.ui.CheckBox$CheckBoxStyle: {
            default: { checkboxOff: check-off, checkboxOn: check-on,
              checkboxOffDisabled: check-disabled, fontColor: ink,
              font: default-font }
          },
          com.badlogic.gdx.scenes.scene2d.ui.Slider$SliderStyle: {
            default-horizontal: { background: slider-track,
              knobBefore: slider-fill, knob: slider-knob }
          },
          com.badlogic.gdx.scenes.scene2d.ui.Window$WindowStyle: {
            default: { background: window, titleFontColor: ink,
              titleFont: default-font }
          }
        })skin";
        std::unordered_map<std::string, DrawablePtr> resolved;
        SkinLoadReport report;
        Skin imported;
        const bool loaded = load_libgdx_skin(
            imported,
            relaxed_skin,
            [&resolved](std::string_view name) {
                auto drawable = std::make_shared<ColorDrawable>(
                    squared::graphics::Color::white()
                );
                resolved.emplace(std::string(name), drawable);
                return drawable;
            },
            report
        );
        require(loaded && report.success(),
                "relaxed libGDX skin JSON loads transactionally");
        require(report.styles_loaded == 5 && report.colors_loaded == 2,
                "supported style and color resources are counted");
        require(imported.button_style("default").normal == resolved["button-up"] &&
                    imported.text_field_style("default").focused ==
                        resolved["field-focus"] &&
                    imported.check_box_style("default").checked ==
                        resolved["check-on"] &&
                    imported.slider_style("default-horizontal").knob ==
                        resolved["slider-knob"] &&
                    imported.window_style("default").background ==
                        resolved["window"],
                "libGDX fields map to primitive Squared styles");

        const DrawablePtr original = imported.button_style("default").normal;
        SkinLoadReport invalid_report;
        require(!load_libgdx_skin(
                    imported,
                    R"({com.badlogic.gdx.graphics.g2d.BitmapFont:{bad:{file:../escape.fnt}},com.badlogic.gdx.scenes.scene2d.ui.TextButton$TextButtonStyle:{default:{up:missing}}})",
                    [](std::string_view) { return DrawablePtr{}; },
                    invalid_report
                ) &&
                    imported.button_style("default").normal == original,
                "invalid paths and resources cannot partially replace a skin");
    }
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

    squared::graphics2d::TextureRegion patch_region(30, 30);
    NinePatchDrawable nine_patch(
        patch_region,
        NinePatchSplits{5, 6, 7, 8},
        Insets{3, 4, 5, 6}
    );
    require(nine_patch.minimum_size().width == 12.0F &&
                nine_patch.minimum_size().height == 14.0F,
            "nine-patch minimum size preserves fixed borders");
    require(nine_patch.content_insets().left == 3.0F &&
                nine_patch.content_insets().bottom == 6.0F,
            "nine-patch accepts independent content insets");
    const std::size_t patch_start = painter.regions.size();
    nine_patch.draw(painter, {10, 20, 60, 50});
    require(painter.regions.size() == patch_start + 9,
            "nine-patch emits all nine drawable regions");
    require(painter.regions[patch_start].width == 5.0F &&
                painter.regions[patch_start].height == 6.0F &&
                painter.regions[patch_start + 4].width == 48.0F &&
                painter.regions[patch_start + 4].height == 36.0F &&
                painter.regions[patch_start + 8].width == 7.0F &&
                painter.regions[patch_start + 8].height == 8.0F,
            "nine-patch preserves corners and stretches its center");
    require(painter.region_sizes[patch_start].width == 5.0F &&
                painter.region_sizes[patch_start + 4].width == 18.0F &&
                painter.region_sizes[patch_start + 8].height == 8.0F,
            "nine-patch slices source regions at the configured splits");
    const std::size_t small_patch_start = painter.regions.size();
    nine_patch.draw(painter, {0, 0, 6, 7});
    require(painter.regions[small_patch_start].width == 2.5F &&
                painter.regions[small_patch_start + 1].width == 3.5F &&
                painter.regions[small_patch_start + 2].height == 4.0F,
            "nine-patch proportionally scales borders in undersized bounds");

    Ui ui(320.0F, 240.0F, std::move(skin));
    RecordingTextInput text_input_service;
    ui.set_text_input_service(&text_input_service);
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
    require(text_input_service.active() && text_input_service.starts == 1,
            "text-field focus requests platform text input");
    require(text_input_service.last_area.width == field_pointer->width(),
            "text-input request exposes the focused field area");
    event.type = squared::application::Event::Type::TextEditing;
    event.text = "d";
    event.editing_start = 0;
    event.editing_length = 1;
    require(ui.event(event), "IME composition reaches the focused field");
    event.type = squared::application::Event::Type::TextInput;
    event.text = " disk";
    require(ui.event(event), "committed platform text reaches the focused field");
    require(field_pointer->text() == "changed disk", "text insertion is retained");
    event.type = squared::application::Event::Type::KeyDown;
    event.key = squared::application::Event::Key::backspace;
    require(ui.event(event), "portable key event reaches the focused field");
    require(field_pointer->text() == "changed dis", "backspace edits text");

    event.type = squared::application::Event::Type::PointerDown;
    event.y = check_pointer->y() + 10.0F;
    require(ui.event(event), "checkbox accepts framework event");
    require(!text_input_service.active() && text_input_service.stops == 1,
            "focus leaving a text field hides platform text input");
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
    require(ui.key_down(Key::space), "space activates a focused button");
    require(clicks == 3, "space activation runs callback once");

    bool navigation_intercepted = false;
    const auto navigation_listener = button_pointer->add_input_listener(
        [&navigation_intercepted](squared::scene2d::InputEvent& input) {
            if (input.type == squared::scene2d::InputType::navigation &&
                input.navigation ==
                    squared::scene2d::NavigationAction::next &&
                input.input_device_id == 42) {
                navigation_intercepted = true;
                input.handle();
                input.stop();
            }
        }
    );
    require(
        ui.navigation(squared::application::Event::Navigation::next, 42),
        "focused actors may intercept portable semantic navigation"
    );
    require(navigation_intercepted && ui.focused() == button_pointer,
            "navigation listeners receive device identity before fallback");
    require(button_pointer->remove_input_listener(navigation_listener),
            "portable navigation listeners can be removed");

    bool key_release_intercepted = false;
    const auto key_release_listener = button_pointer->add_input_listener(
        [&key_release_intercepted](squared::scene2d::InputEvent& input) {
            if (input.type == squared::scene2d::InputType::key_up &&
                input.key == squared::scene2d::InputKey::space &&
                input.modifiers.control) {
                key_release_intercepted = true;
                input.handle();
            }
        }
    );
    event.type = squared::application::Event::Type::KeyUp;
    event.key = squared::application::Event::Key::space;
    event.modifiers = {};
    event.modifiers.set(squared::application::KeyModifier::control);
    require(ui.event(event) && key_release_intercepted,
            "portable key releases propagate with modifier state");
    require(button_pointer->remove_input_listener(key_release_listener),
            "portable key-release listeners can be removed");

    event.type = squared::application::Event::Type::KeyDown;
    event.key = squared::application::Event::Key::tab;
    event.modifiers = {};
    require(ui.event(event), "Tab advances portable focus");
    require(ui.focused() == field_pointer, "Tab wraps to the first focusable widget");
    event.modifiers.set(squared::application::KeyModifier::shift);
    require(ui.event(event), "Shift+Tab reverses portable focus");
    require(ui.focused() == button_pointer, "Shift+Tab wraps in reverse");
    event.modifiers = {};
    event.key = squared::application::Event::Key::up;
    require(ui.event(event), "directional keyboard input moves focus");
    require(ui.focused() == slider_pointer,
            "directional focus selects the nearest widget above");
    event.type = squared::application::Event::Type::NavigationInput;
    event.navigation = squared::application::Event::Navigation::up;
    require(ui.event(event), "controller-style navigation moves focus");
    require(ui.focused() == check_pointer,
            "semantic navigation is independent of controller API constants");
    require(ui.navigation(squared::application::Event::Navigation::next) &&
                ui.navigation(squared::application::Event::Navigation::next),
            "semantic next navigation follows traversal order");
    require(ui.focused() == button_pointer,
            "semantic next navigation reaches the button");

    auto stack = std::make_unique<Stack>();
    stack->set_size(100.0F, 80.0F);
    Widget& stacked = stack->add(std::make_unique<Panel>());
    stack->validate_layout(painter, ui.skin());
    require(stacked.width() == 100.0F && stacked.height() == 80.0F,
            "stack fills every child");

    Table grid;
    grid.set_padding(0.0F);
    grid.set_spacing(0.0F);
    grid.set_size(300.0F, 120.0F);
    auto first = std::make_unique<Button>("A");
    auto spanning = std::make_unique<Button>("B");
    auto wide = std::make_unique<TextField>("three columns");
    Widget* first_pointer = first.get();
    Widget* spanning_pointer = spanning.get();
    Widget* wide_pointer = wide.get();
    grid.add(std::move(first)).grow_x().fill_x();
    grid.add(std::move(spanning)).column_span(2).grow_x().fill_x();
    grid.row();
    grid.add(std::move(wide)).column_span(3).grow().fill();
    grid.validate_layout(painter, ui.skin());
    require(first_pointer->width() > 100.0F,
            "table distributes grow weight across columns");
    require(spanning_pointer->x() == first_pointer->width() &&
                spanning_pointer->x() + spanning_pointer->width() == 300.0F,
            "table column spans occupy adjacent columns");
    require(wide_pointer->width() == 300.0F,
            "table fill honors a full-row column span");

    auto floating = std::make_unique<Window>("Inventory");
    floating->set_bounds(10.0F, 10.0F, 160.0F, 100.0F);
    floating->set_closable(true);
    floating->set_resizable(true);
    floating->set_minimum_window_size({140.0F, 90.0F});
    floating->content_table().add(std::make_unique<Label>("Backpack"));
    Window* floating_pointer = floating.get();
    ui.show_window(std::move(floating), false);
    ui.layout(painter);
    require(ui.pointer(PointerAction::down, 20.0F, 20.0F, 0, 11),
            "window title captures a drag pointer");
    require(ui.pointer(PointerAction::move, 60.0F, 50.0F, 0, 11),
            "captured title receives drag motion");
    ui.pointer(PointerAction::up, 60.0F, 50.0F, 0, 11);
    require(floating_pointer->x() == 50.0F && floating_pointer->y() == 40.0F,
            "window dragging updates its stage position");
    require(ui.pointer(PointerAction::down, 209.0F, 139.0F, 0, 14),
            "window corner captures a resize pointer");
    require(ui.pointer(PointerAction::move, 300.0F, 220.0F, 0, 14),
            "captured corner receives resize motion");
    ui.pointer(PointerAction::up, 300.0F, 220.0F, 0, 14);
    require(floating_pointer->width() == 251.0F &&
                floating_pointer->height() == 181.0F,
            "window corner resizes both dimensions");
    require(ui.pointer(PointerAction::down, 300.0F, 220.0F, 0, 15),
            "resized corner can start another resize");
    ui.pointer(PointerAction::move, 60.0F, 60.0F, 0, 15);
    ui.pointer(PointerAction::up, 60.0F, 60.0F, 0, 15);
    require(floating_pointer->width() >= 140.0F &&
                floating_pointer->height() >= 90.0F,
            "window resizing enforces its minimum size");

    require(ui.pointer(PointerAction::down,
                       floating_pointer->x() + 20.0F,
                       floating_pointer->y() + 18.0F, 0, 16),
            "window title starts a constrained drag");
    ui.pointer(PointerAction::move, -100.0F, -100.0F, 0, 16);
    ui.pointer(PointerAction::up, -100.0F, -100.0F, 0, 16);
    require(floating_pointer->x() == 0.0F && floating_pointer->y() == 0.0F,
            "window dragging is clamped to the top-left UI bounds");
    require(ui.pointer(PointerAction::down, 20.0F, 18.0F, 0, 17),
            "clamped window remains draggable");
    ui.pointer(PointerAction::move, 500.0F, 500.0F, 0, 17);
    ui.pointer(PointerAction::up, 500.0F, 500.0F, 0, 17);
    require(floating_pointer->x() + floating_pointer->width() == 320.0F &&
                floating_pointer->y() + floating_pointer->height() == 240.0F,
            "window dragging is clamped to the bottom-right UI bounds");

    ui.resize(200.0F, 150.0F);
    ui.layout(painter);
    require(floating_pointer->x() + floating_pointer->width() <= 200.0F &&
                floating_pointer->y() + floating_pointer->height() <= 150.0F,
            "viewport resizing keeps a window within the available UI bounds");
    ui.resize(320.0F, 240.0F);
    ui.layout(painter);

    const float close_x = floating_pointer->x() + floating_pointer->width() - 18.0F;
    const float close_y = floating_pointer->y() + 18.0F;
    require(ui.pointer(PointerAction::down, close_x, close_y, 0, 18),
            "window close control captures pointer down");
    require(ui.pointer(PointerAction::up, close_x, close_y, 0, 18),
            "window close control handles pointer up");
    require(ui.window_count() == 0,
            "window close control removes the floating window safely");

    std::string dialog_result;
    auto dialog = std::make_unique<Dialog>(
        "Mount cartridge",
        [&dialog_result](std::string_view result) { dialog_result = result; }
    );
    dialog->text("Mount read-only?").button("Cancel", "cancel").button("Mount", "mount");
    Dialog* dialog_pointer = dialog.get();
    ui.show_dialog(std::move(dialog));
    ui.layout(painter);
    auto* cancel_button = dynamic_cast<Button*>(
        dialog_pointer->button_table().child_at(0)
    );
    auto* mount_button = dynamic_cast<Button*>(
        dialog_pointer->button_table().child_at(1)
    );
    require(cancel_button && mount_button,
            "dialog convenience method creates action buttons");
    require(ui.focused() == cancel_button,
            "opening a modal focuses its first control");
    require(ui.key_down(Key::tab) && ui.focused() == mount_button,
            "Tab traversal is trapped inside the modal");
    require(ui.key_down(Key::tab, {.shift = true}) &&
                ui.focused() == cancel_button,
            "Shift+Tab reverses inside the modal scope");
    const std::size_t fills_before_modal_paint = painter.fills.size();
    ui.paint(painter);
    require(std::any_of(
                painter.fills.begin() +
                    static_cast<std::ptrdiff_t>(fills_before_modal_paint),
                painter.fills.end(),
                [](const Rectangle& rectangle) {
                    return rectangle.x == 0.0F && rectangle.y == 0.0F &&
                        rectangle.width == 320.0F && rectangle.height == 240.0F;
                }),
            "modal dialog paints a stage-sized dimming layer");
    const int clicks_before_modal_test = clicks;
    event.type = squared::application::Event::Type::PointerDown;
    event.pointer_id = 12;
    event.x = 20.0F;
    event.y = button_pointer->y() + 10.0F;
    require(ui.event(event), "modal dialog consumes input outside its bounds");
    event.type = squared::application::Event::Type::PointerUp;
    ui.event(event);
    require(clicks == clicks_before_modal_test,
            "modal dialog blocks controls beneath it");

    float action_x = mount_button->width() * 0.5F;
    float action_y = mount_button->height() * 0.5F;
    for (const squared::scene2d::Actor* actor = mount_button;
         actor && actor->parent(); actor = actor->parent()) {
        action_x += actor->x();
        action_y += actor->y();
    }
    event.type = squared::application::Event::Type::PointerDown;
    event.pointer_id = 13;
    event.x = action_x;
    event.y = action_y;
    require(ui.event(event), "dialog action accepts pointer down");
    event.type = squared::application::Event::Type::PointerUp;
    require(ui.event(event), "dialog action accepts pointer up");
    require(dialog_result == "mount", "dialog reports the selected result");
    require(ui.focused() == button_pointer,
            "closing a modal dialog restores the previous focus");

    auto escape_dialog = std::make_unique<Dialog>("Discard changes?");
    escape_dialog->text("Unsaved changes will be lost.");
    ui.show_dialog(std::move(escape_dialog));
    ui.layout(painter);
    require(ui.key_down(Key::escape), "escape closes a dialog by default");
    require(ui.focused() == button_pointer,
            "escape dismissal restores the previous focus");

    ui.paint(painter);
    require(painter.clip_depth == 0, "paint clips are balanced");
    require(!painter.fills.empty(), "skin drawables use painter commands");
    require(painter.strings.size() >= 4, "basic controls emit text");

    squared::graphics2d::TextureRegion empty_region;
    RegionDrawable region_drawable(empty_region);
    const std::size_t region_draw_start = painter.regions.size();
    region_drawable.draw(painter, {0, 0, 16, 16});
    require(painter.regions.size() == region_draw_start + 1,
            "region skins draw through Graphics2D TextureRegion");

    std::cout << "Squared GUI skins, controls, layout, and framework events: OK\n";
    return 0;
}
