#include <squared_gui_showcase/showcase.hpp>

#include <iomanip>
#include <sstream>
#include <utility>

namespace squared_gui_showcase {
namespace {

using squared::gui::Alignment;
using squared::gui::Button;
using squared::gui::ButtonGroup;
using squared::gui::CheckBox;
using squared::gui::Dialog;
using squared::gui::Direction;
using squared::gui::Image;
using squared::gui::Insets;
using squared::gui::Label;
using squared::gui::LinearLayout;
using squared::gui::MarginContainer;
using squared::gui::Panel;
using squared::gui::ProgressBar;
using squared::gui::RadioButton;
using squared::gui::ScrollPane;
using squared::gui::Separator;
using squared::gui::Slider;
using squared::gui::Stack;
using squared::gui::Table;
using squared::gui::TextField;
using squared::gui::ToggleButton;
using squared::gui::Window;

std::string volume_text(float value)
{
    std::ostringstream output;
    output << "Volume: " << std::fixed << std::setprecision(0) << value << '%';
    return output.str();
}

} // namespace

Showcase::Showcase(
    float width,
    float height,
    squared::gui::Skin skin,
    squared::gui::DrawablePtr image
)
    : ui_(width, height, std::move(skin)), image_(std::move(image))
{
    ui_.set_content(make_launcher());
    show_controls_window();
    show_layout_window();
    show_inspector_window();
    refresh_status();
}

std::unique_ptr<squared::gui::Widget> Showcase::make_launcher()
{
    auto stack = std::make_unique<Stack>();
    stack->add(std::make_unique<Panel>());

    auto table = std::make_unique<Table>();
    table->set_padding(18.0F);
    table->set_spacing(10.0F);
    table->add(std::make_unique<Label>("Squared GUI Showcase"))
        .column_span(4).align(Alignment::start).grow_x().fill_x();
    table->row();
    auto summary = std::make_unique<Label>(
        "Drag and resize windows. Every launcher may open another copy."
    );
    summary->set_muted(true);
    table->add(std::move(summary)).column_span(4)
        .align(Alignment::start).grow_x().fill_x();
    table->row();
    table->add(std::make_unique<Separator>())
        .column_span(4).grow_x().fill_x().pad(4.0F);
    table->row();
    auto controls = std::make_unique<Button>("Controls", [this] {
        show_controls_window();
    });
    controls->set_glyph("C");
    controls->set_tooltip("Open another complete controls window");
    table->add(std::move(controls)).fill_x();
    auto layouts = std::make_unique<Button>("Layouts", [this] {
        show_layout_window();
    });
    layouts->set_glyph("L");
    layouts->set_tooltip("Open the scrolling layout gallery");
    table->add(std::move(layouts)).fill_x();
    auto inspector = std::make_unique<Button>("Inspector", [this] {
        show_inspector_window();
    });
    inspector->set_glyph("I");
    inspector->set_tooltip("Open a compact floating-window inspector");
    table->add(std::move(inspector)).fill_x();
    auto modal = std::make_unique<Button>("Modal dialog", [this] {
        show_confirmation_dialog();
    });
    modal->set_glyph("!");
    modal->set_tooltip("Show a modal-scoped confirmation dialog");
    table->add(std::move(modal)).fill_x();
    table->row();

    auto status = std::make_unique<Label>(status_);
    status_label_ = status.get();
    status->set_muted(true);
    table->add(std::move(status)).column_span(4)
        .align(Alignment::start).grow_x().fill_x().pad({0, 8, 0, 0});
    table->row();
    table->add(std::make_unique<Panel>()).column_span(4).grow().fill();

    auto margin = std::make_unique<MarginContainer>(
        Insets{12.0F, 12.0F, 12.0F, 12.0F}
    );
    margin->set_content(std::move(table));
    stack->add(std::move(margin));
    return stack;
}

std::unique_ptr<Window> Showcase::make_controls_window()
{
    auto window = std::make_unique<Window>("Widget controls");
    window->set_bounds(36.0F, 66.0F, 350.0F, 460.0F);
    window->set_closable(true);
    window->set_resizable(true);
    window->set_minimum_window_size({310.0F, 330.0F});
    Table& table = window->content_table();
    table.set_spacing(8.0F);

    if (image_) {
        table.add(std::make_unique<Image>(image_))
            .column_span(2).fill_x().pad(4.0F);
        table.row();
    }
    table.add(std::make_unique<Label>("Player name"))
        .align(Alignment::end);
    auto name = std::make_unique<TextField>("Squared Netizen");
    name_field_ = name.get();
    table.add(std::move(name)).grow_x().fill_x();
    table.row();

    auto music = std::make_unique<CheckBox>("Music enabled", true);
    music->set_on_change([this](bool checked) {
        set_status(checked ? "Music enabled" : "Music disabled");
    });
    table.add(std::move(music)).column_span(2)
        .align(Alignment::start).grow_x().fill_x();
    table.row();

    auto challenge = std::make_unique<ToggleButton>("Challenge mode", false);
    challenge->set_on_change([this](bool checked) {
        set_status(checked ? "Challenge mode selected" : "Normal mode selected");
    });
    table.add(std::move(challenge)).column_span(2).grow_x().fill_x();
    table.row();

    auto difficulty = std::make_unique<ButtonGroup>(1, 1);
    auto casual = std::make_unique<RadioButton>("Casual", true);
    auto expert = std::make_unique<RadioButton>("Expert");
    difficulty->add(*casual);
    difficulty->add(*expert);
    casual->set_on_change([this](bool checked) {
        if (checked) set_status("Casual difficulty selected");
    });
    expert->set_on_change([this](bool checked) {
        if (checked) set_status("Expert difficulty selected");
    });
    table.add(std::move(casual)).grow_x().fill_x();
    table.add(std::move(expert)).grow_x().fill_x();
    table.row();
    button_groups_.push_back(std::move(difficulty));

    auto volume = std::make_unique<Label>(volume_text(65.0F));
    volume_label_ = volume.get();
    table.add(std::move(volume)).align(Alignment::end);
    auto progress = std::make_unique<ProgressBar>(0.0F, 100.0F, 65.0F);
    ProgressBar* progress_pointer = progress.get();
    auto slider = std::make_unique<Slider>(0.0F, 100.0F, 65.0F);
    slider->set_step(5.0F);
    slider->set_on_change([this, progress_pointer](float value) {
        if (volume_label_) volume_label_->set_text(volume_text(value));
        progress_pointer->set_value(value);
    });
    table.add(std::move(slider)).grow_x().fill_x();
    table.row();
    table.add(std::make_unique<Label>("Progress")).align(Alignment::end);
    table.add(std::move(progress)).grow_x().fill_x();
    table.row();
    table.add(std::make_unique<Separator>())
        .column_span(2).grow_x().fill_x().pad(4.0F);
    table.row();

    table.add(std::make_unique<Button>("Apply", [this] {
        show_confirmation_dialog();
    })).grow_x().fill_x();
    table.add(std::make_unique<Button>("About", [this] {
        show_about_dialog();
    })).grow_x().fill_x();
    table.row();
    table.add(std::make_unique<Button>("Reset name", [this] {
        if (name_field_) name_field_->set_text("Squared Netizen");
        set_status("Control values reset");
    })).column_span(2).grow_x().fill_x();
    return window;
}

std::unique_ptr<Window> Showcase::make_layout_window()
{
    auto window = std::make_unique<Window>("Layout gallery");
    window->set_bounds(405.0F, 126.0F, 300.0F, 350.0F);
    window->set_closable(true);
    window->set_resizable(true);
    window->set_minimum_window_size({260.0F, 260.0F});

    auto list = std::make_unique<LinearLayout>(Direction::vertical);
    list->set_padding(8.0F);
    list->set_spacing(7.0F);
    list->add(std::make_unique<Label>("Scrollable LinearLayout"));
    list->add(std::make_unique<Separator>());
    for (int index = 1; index <= 10; ++index) {
        list->add(std::make_unique<Button>(
            "Open dialog from row " + std::to_string(index),
            [this, index] {
                set_status("Layout row " + std::to_string(index) + " selected");
                show_confirmation_dialog();
            }
        ));
    }
    list->add(std::make_unique<Separator>());
    auto note = std::make_unique<Label>("Drag vertically here to scroll.");
    note->set_muted(true);
    list->add(std::move(note));

    auto scroll = std::make_unique<ScrollPane>();
    scroll->set_content(std::move(list));
    window->content_table().add(std::move(scroll)).grow().fill();
    return window;
}

std::unique_ptr<Window> Showcase::make_inspector_window()
{
    auto window = std::make_unique<Window>("Window inspector");
    window->set_bounds(724.0F, 96.0F, 210.0F, 280.0F);
    window->set_closable(true);
    window->set_resizable(true);
    window->set_minimum_window_size({190.0F, 230.0F});
    Table& table = window->content_table();
    table.set_spacing(7.0F);
    table.add(std::make_unique<Label>("Floating windows"))
        .column_span(2).grow_x().fill_x();
    table.row();
    table.add(std::make_unique<Separator>(Direction::vertical)).fill_y();
    auto hint = std::make_unique<Label>("Close, drag, or resize windows.");
    hint->set_muted(true);
    table.add(std::move(hint)).grow().fill();
    table.row();
    table.add(std::make_unique<Button>("Confirm", [this] {
        show_confirmation_dialog();
    })).column_span(2).grow_x().fill_x();
    table.row();
    table.add(std::make_unique<Button>("About", [this] {
        show_about_dialog();
    })).column_span(2).grow_x().fill_x();
    return window;
}

void Showcase::show_controls_window()
{
    ui_.show_window(make_controls_window(), false);
    refresh_status();
}

void Showcase::show_layout_window()
{
    ui_.show_window(make_layout_window(), false);
    refresh_status();
}

void Showcase::show_inspector_window()
{
    ui_.show_window(make_inspector_window(), false);
    refresh_status();
}

void Showcase::show_confirmation_dialog()
{
    auto dialog = std::make_unique<Dialog>(
        "Apply settings?",
        [this](std::string_view result) {
            set_status(result == "apply"
                ? "Settings applied from modal dialog"
                : "Modal dialog cancelled");
        }
    );
    dialog->set_closable(true);
    dialog->set_minimum_window_size({330.0F, 180.0F});
    dialog->text("This modal blocks every window behind it.")
        .button("Cancel", "cancel")
        .button("Apply", "apply");
    ui_.show_dialog(std::move(dialog));
    refresh_status();
}

void Showcase::show_about_dialog()
{
    auto dialog = std::make_unique<Dialog>(
        "About Squared GUI",
        [this](std::string_view) { set_status("About dialog closed"); }
    );
    dialog->set_closable(true);
    dialog->set_minimum_window_size({380.0F, 200.0F});
    dialog->text("Portable retained-mode GUI")
        .text("Tables, skins, windows, dialogs, and controls")
        .button("Close", "close");
    ui_.show_dialog(std::move(dialog));
    refresh_status();
}

void Showcase::refresh_status()
{
    set_status(
        "Active floating layers: " + std::to_string(ui_.window_count())
    );
}

void Showcase::report_graphics_recovery(
    std::uint64_t generation,
    bool resources_preserved
)
{
    set_status(
        "Graphics generation " + std::to_string(generation) +
        (resources_preserved ? " preserved" : " rebuilt")
    );
}

void Showcase::set_status(std::string status)
{
    status_ = std::move(status);
    if (status_label_) status_label_->set_text(status_);
}

} // namespace squared_gui_showcase
