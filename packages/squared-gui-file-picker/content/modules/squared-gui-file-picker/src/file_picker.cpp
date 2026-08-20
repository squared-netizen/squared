#include <squared/gui/file_picker.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <utility>

namespace squared::gui {
namespace {

holodisk::Status failure(holodisk::ErrorCode code, std::string message)
{
    return holodisk::Status::failure({code, std::move(message)});
}

std::string join_path(std::string_view parent, std::string_view child)
{
    if (parent == "/") return "/" + std::string(child);
    return std::string(parent) + "/" + std::string(child);
}

std::string parent_path(std::string_view path)
{
    if (path.empty() || path == "/") return "/";
    const std::size_t slash = path.find_last_of('/');
    return slash == 0 || slash == std::string_view::npos
        ? "/" : std::string(path.substr(0, slash));
}

} // namespace

holodisk::Status register_file_picker_asset_loaders(
    holodisk::AssetManager& manager
)
{
    return manager.register_loader<FilePickerTextAsset>(
        [](holodisk::AssetLoadContext& context, std::string_view path)
            -> holodisk::Result<holodisk::AssetHandle<FilePickerTextAsset>> {
            auto bytes = context.read_bytes(path);
            if (!bytes) {
                return holodisk::Result<holodisk::AssetHandle<FilePickerTextAsset>>::failure(
                    bytes.error()
                );
            }
            auto asset = std::make_shared<FilePickerTextAsset>();
            asset->text.reserve(bytes.value().size());
            for (const std::byte value : bytes.value()) {
                asset->text.push_back(static_cast<char>(std::to_integer<unsigned char>(value)));
            }
            return holodisk::Result<holodisk::AssetHandle<FilePickerTextAsset>>::success(
                std::move(asset)
            );
        }
    );
}

bool load_file_picker_holo_skin(
    holodisk::AssetManager& manager,
    Skin& destination,
    std::string_view json_path,
    const SkinDrawableResolver& drawable_resolver,
    const SkinFontResolver& font_resolver,
    SkinLoadReport& report,
    const SkinLoadLimits& limits
) noexcept
{
    report = {};
    try {
        const holodisk::Status registration = register_file_picker_asset_loaders(manager);
        if (!registration && registration.error().code != holodisk::ErrorCode::AlreadyExists) {
            report.issues.push_back({
                SkinLoadSeverity::error, std::string(json_path), registration.error().message
            });
            return false;
        }
        auto document = manager.load<FilePickerTextAsset>(json_path);
        if (!document) {
            report.issues.push_back({
                SkinLoadSeverity::error, std::string(json_path), document.error().message
            });
            return false;
        }
        return load_libgdx_skin(
            destination,
            document.value()->text,
            drawable_resolver,
            font_resolver,
            report,
            limits
        );
    } catch (const std::exception& exception) {
        report.issues.push_back({
            SkinLoadSeverity::error, std::string(json_path), exception.what()
        });
    } catch (...) {
        report.issues.push_back({
            SkinLoadSeverity::error, std::string(json_path), "unknown asset-loader failure"
        });
    }
    return false;
}

FilePicker::FilePicker(
    holodisk::HoloDrive& drive,
    holodisk::AssetManager& assets,
    FilePickerOptions options,
    ResultCallback result
)
    : Window(options.title, options.window_style),
      drive_(&drive),
      assets_(&assets),
      options_(std::move(options)),
      result_(std::move(result))
{
    set_modal(true);
    set_closable(false);
    set_resizable(true);
    set_minimum_window_size({420.0F, 320.0F});

    auto path = std::make_unique<TextField>(options_.initial_path);
    path->set_style(options_.text_field_style);
    path_field_ = path.get();
    content_table().add(std::move(path)).grow_x().fill_x();

    auto go = std::make_unique<Button>("Go", [this] {
        [[maybe_unused]] const auto status = navigate_to(path_field_->text());
    });
    go->set_style(options_.button_style);
    go->set_tooltip("Open the virtual HoloDrive directory in the path field");
    content_table().add(std::move(go));
    content_table().row();

    auto up = std::make_unique<Button>("Up", [this] {
        [[maybe_unused]] const auto status = navigate_up();
    });
    up->set_style(options_.button_style);
    up->set_glyph("↑");
    content_table().add(std::move(up));

    auto refresh_button = std::make_unique<Button>("Refresh", [this] {
        [[maybe_unused]] const auto status = refresh();
    });
    refresh_button->set_style(options_.button_style);
    refresh_button->set_glyph("↻");
    content_table().add(std::move(refresh_button));
    content_table().row();

    auto pane = std::make_unique<ScrollPane>();
    entries_pane_ = pane.get();
    content_table().add(std::move(pane)).column_span(2).grow().fill();
    content_table().row();

    auto status = std::make_unique<Label>();
    status->set_style(options_.label_style);
    status->set_muted(true);
    status_label_ = status.get();
    content_table().add(std::move(status)).column_span(2).grow_x().fill_x();
    content_table().row();

    auto cancel_button = std::make_unique<Button>("Cancel", [this] { cancel(); });
    cancel_button->set_style(options_.button_style);
    content_table().add(std::move(cancel_button));

    auto choose = std::make_unique<Button>("Choose", [this] {
        static_cast<void>(accept());
    });
    choose->set_style(options_.button_style);
    choose->set_glyph("✓");
    content_table().add(std::move(choose));

    [[maybe_unused]] const auto initial = navigate_to(options_.initial_path);
}

holodisk::Status FilePicker::refresh()
{
    return navigate_to(current_path_);
}

holodisk::Status FilePicker::navigate_to(std::string_view path)
{
    auto listed = drive_->list(path);
    if (!listed) {
        report_error(listed.error());
        return holodisk::Status::failure(listed.error());
    }
    current_path_ = std::string(path);
    if (current_path_.size() > 1 && current_path_.back() == '/') {
        current_path_.pop_back();
    }
    entries_ = std::move(listed).value();
    selection_.reset();
    last_error_.clear();
    path_field_->set_text(current_path_);
    rebuild_entry_widgets();
    update_status();
    invalidate_layout();
    return holodisk::Status::success();
}

holodisk::Status FilePicker::navigate_up()
{
    return navigate_to(parent_path(current_path_));
}

holodisk::Status FilePicker::select_entry(std::string_view name)
{
    const auto found = std::find_if(entries_.begin(), entries_.end(),
        [name](const holodisk::Entry& entry) { return entry.name == name; });
    if (found == entries_.end()) {
        const auto status = failure(holodisk::ErrorCode::NotFound, "entry is not visible");
        report_error(status.error());
        return status;
    }
    selection_ = *found;
    last_error_.clear();
    update_status();
    return holodisk::Status::success();
}

holodisk::Status FilePicker::activate_entry(std::string_view name)
{
    const holodisk::Status selected = select_entry(name);
    if (!selected || !selection_ || !selection_->directory) return selected;
    return navigate_to(join_path(current_path_, selection_->name));
}

bool FilePicker::accept()
{
    if (completed_) return false;
    bool directory = false;
    std::string path;
    if (selection_) {
        directory = selection_->directory;
        path = selected_path();
    } else if (options_.selection_mode != FilePickerSelectionMode::files) {
        directory = true;
        path = current_path_;
    } else {
        last_error_ = "Choose a file";
        update_status();
        return false;
    }
    if (!selection_allowed(directory)) {
        last_error_ = directory ? "Choose a file" : "Choose a directory";
        update_status();
        return false;
    }
    completed_ = true;
    request_close();
    if (result_) result_({true, std::move(path), directory});
    return true;
}

void FilePicker::cancel()
{
    if (completed_) return;
    completed_ = true;
    request_close();
    if (result_) result_({false, {}, false});
}

std::string FilePicker::selected_path() const
{
    return selection_ ? join_path(current_path_, selection_->name) : std::string{};
}

bool FilePicker::selection_allowed(bool directory) const noexcept
{
    return options_.selection_mode == FilePickerSelectionMode::files_and_directories ||
        (directory && options_.selection_mode == FilePickerSelectionMode::directories) ||
        (!directory && options_.selection_mode == FilePickerSelectionMode::files);
}

void FilePicker::rebuild_entry_widgets()
{
    auto rows = std::make_unique<LinearLayout>(Direction::vertical);
    rows->set_spacing(2.0F);
    for (const holodisk::Entry& entry : entries_) {
        const std::string name = entry.name;
        const bool directory = entry.directory;
        auto button = std::make_unique<Button>(directory ? name + "/" : name, [this, name, directory] {
            if (directory) {
                [[maybe_unused]] const auto status = navigate_to(join_path(current_path_, name));
            } else {
                [[maybe_unused]] const auto status = select_entry(name);
            }
        });
        button->set_style(options_.button_style);
        button->set_glyph(directory ? "▸" : "•");
        button->set_tooltip(directory ? "Open directory" : "Select file");
        static_cast<void>(rows->add(std::move(button)));
    }
    static_cast<void>(entries_pane_->set_content(std::move(rows)));
    entries_pane_->set_scroll_y(0.0F);
}

void FilePicker::update_status()
{
    if (!last_error_.empty()) {
        status_label_->set_text(last_error_);
    } else if (selection_) {
        status_label_->set_text(selected_path());
    } else {
        status_label_->set_text(std::to_string(entries_.size()) + " entries");
    }
}

void FilePicker::report_error(const holodisk::Error& error)
{
    last_error_ = error.message.empty() ? "HoloDrive operation failed" : error.message;
    update_status();
}

} // namespace squared::gui
