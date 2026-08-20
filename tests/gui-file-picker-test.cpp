#include <squared/gui/file_picker.hpp>

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace {

using namespace squared;
namespace fs = std::filesystem;

[[noreturn]] void fail(std::string_view message)
{
    std::cerr << message << '\n';
    std::exit(1);
}

template<typename Condition>
void require(Condition&& condition, std::string_view message)
{
    if (!static_cast<bool>(condition)) fail(message);
}

std::span<const std::byte> bytes(std::string_view text)
{
    return {reinterpret_cast<const std::byte*>(text.data()), text.size()};
}

void write_file(holodisk::HoloDrive& drive, std::string_view path, std::string_view text)
{
    auto file = drive.open(path, holodisk::OpenMode::WriteTruncate);
    require(file, "test file did not open");
    auto written = drive.write(file.value(), bytes(text));
    require(written && written.value() == text.size(), "test file did not write");
    require(drive.close(file.value()), "test file did not close");
}

} // namespace

int main()
{
    std::error_code ignored;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path root = fs::temp_directory_path(ignored) /
        ("squared-file-picker-test-" + std::to_string(nonce));
    fs::remove_all(root, ignored);
    fs::create_directories(root, ignored);
    require(!ignored, "test scratch directory was not created");

    auto factory = holodisk::make_standard_holodrive_factory();
    holodisk::DriveOptions options;
    options.scratch_directory = (root / "scratch").string();
    auto created = factory->create(options);
    require(created, "HoloDrive was not created");
    auto drive = std::move(created).value();
    auto disk = drive->create_holodisk();
    require(disk, "test HoloDisk was not created");
    auto mount = drive->mount(disk.value(), "/", holodisk::MountAccess::ReadWrite);
    require(mount, "test HoloDisk was not mounted");

    write_file(*drive, "/notes.txt", "hello picker");
    write_file(*drive, "/folder/inside.txt", "nested");
    write_file(*drive, "/skin.json",
        "{com.badlogic.gdx.graphics.Color:{white:{r:1,g:1,b:1,a:1}}}");

    holodisk::AssetManager assets(*drive);
    require(gui::register_file_picker_asset_loaders(assets),
            "file-picker text loader did not register");

    std::optional<gui::FilePickerResult> result;
    gui::FilePicker picker(*drive, assets, {},
        [&result](const gui::FilePickerResult& value) { result = value; });
    require(picker.current_path() == "/", "picker did not open root");
    require(picker.entries().size() == 3, "picker root listing is incomplete");

    require(picker.activate_entry("folder"), "picker did not enter directory");
    require(picker.current_path() == "/folder", "picker directory path is wrong");
    require(picker.entries().size() == 1 && picker.entries()[0].name == "inside.txt",
            "picker nested listing is wrong");
    require(picker.navigate_up(), "picker did not navigate upward");
    require(picker.current_path() == "/", "picker did not return to root");

    const std::string before_invalid = picker.current_path();
    require(!picker.navigate_to("/missing"), "missing path unexpectedly opened");
    require(picker.current_path() == before_invalid,
            "failed navigation changed the committed path");
    require(!picker.last_error().empty(), "failed navigation had no diagnostic");

    require(picker.select_entry("notes.txt"), "file selection failed");
    auto selected_text = picker.load_selected<gui::FilePickerTextAsset>();
    require(selected_text && selected_text.value()->text == "hello picker",
            "selected file did not load through AssetManager");
    require(picker.accept(), "valid file selection was not accepted");
    require(result && result->accepted && result->path == "/notes.txt" &&
                !result->directory,
            "picker returned the wrong accepted result");
    require(picker.close_requested(), "accepted picker did not request closure");

    gui::Skin skin;
    gui::SkinLoadReport report;
    require(gui::load_file_picker_holo_skin(
                assets, skin, "/skin.json",
                [](std::string_view) { return gui::DrawablePtr{}; },
                [](std::string_view, std::string_view) { return gui::FontPtr{}; },
                report),
            "AssetManager-backed skin JSON did not load");
    require(report.success() && report.colors_loaded == 1,
            "skin load report did not record the color");

    gui::FilePickerOptions directory_options;
    directory_options.selection_mode = gui::FilePickerSelectionMode::directories;
    std::optional<gui::FilePickerResult> directory_result;
    gui::FilePicker directory_picker(*drive, assets, directory_options,
        [&directory_result](const gui::FilePickerResult& value) {
            directory_result = value;
        });
    require(directory_picker.accept(), "current directory was not accepted");
    require(directory_result && directory_result->directory &&
                directory_result->path == "/",
            "directory picker returned the wrong result");

    fs::remove_all(root, ignored);
    std::cout << "Squared GUI HoloDisk file picker: OK\n";
}
