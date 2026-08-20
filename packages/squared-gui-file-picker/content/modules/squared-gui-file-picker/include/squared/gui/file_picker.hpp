#pragma once

#include <squared/gui/gui.hpp>
#include <squared/gui/skin_loader.hpp>
#include <squared/holodisk/asset_manager.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace squared::gui {

/** @brief Default virtual path when packaged Android assets are mounted at `/assets`. */
inline constexpr std::string_view gdx_holo_skin_json_path =
    "/assets/gui/gdx-skins/selected/gdx-holo/uiskin.json";

/** @brief Whether a picker may accept files, directories, or either. */
enum class FilePickerSelectionMode { files, directories, files_and_directories };

/** @brief Construction policy for one HoloDisk-backed file picker. */
struct FilePickerOptions final {
    /** @brief Initial absolute HoloDrive directory. */
    std::string initial_path{"/"};
    /** @brief Window title rendered with the selected gdx-holo WindowStyle. */
    std::string title{"Choose file"};
    /** @brief Kinds of entries accepted by the Choose action. */
    FilePickerSelectionMode selection_mode{FilePickerSelectionMode::files};
    /** @brief Window style name from the active Skin. */
    std::string window_style{"default"};
    /** @brief Button style name from the active Skin. */
    std::string button_style{"default"};
    /** @brief Text field style name from the active Skin. */
    std::string text_field_style{"default"};
    /** @brief Label style name from the active Skin. */
    std::string label_style{"default"};
};

/** @brief Result delivered when the user accepts or cancels a picker. */
struct FilePickerResult final {
    /** @brief True for Choose and false for Cancel. */
    bool accepted{false};
    /** @brief Selected absolute virtual path; empty after cancellation. */
    std::string path;
    /** @brief True when the accepted path identifies a directory. */
    bool directory{false};
};

/** @brief UTF-8 text loaded and cached by the HoloDisk AssetManager. */
struct FilePickerTextAsset final {
    /** @brief Complete UTF-8 source document. */
    std::string text;
};

/**
 * @brief Register the text loader used for gdx-holo skin JSON and text files.
 * @param manager Application-owned manager; must outlive returned assets.
 * @return Success, or AlreadyExists when this loader is already registered.
 */
[[nodiscard]] holodisk::Status register_file_picker_asset_loaders(
    holodisk::AssetManager& manager
);

/**
 * @brief Load and transactionally apply a libGDX skin JSON through AssetManager.
 * @param manager Manager bound to a drive containing `json_path`.
 * @param destination Skin replaced only after complete validation.
 * @param json_path Absolute virtual path, normally gdx_holo_skin_json_path.
 * @param drawable_resolver Atlas/drawable strategy owned by the application.
 * @param font_resolver Bitmap-font strategy owned by the application.
 * @param report Replaced with load diagnostics.
 * @param limits Explicit parser and resource limits.
 * @return True when the skin committed; false with destination unchanged.
 */
[[nodiscard]] bool load_file_picker_holo_skin(
    holodisk::AssetManager& manager,
    Skin& destination,
    std::string_view json_path,
    const SkinDrawableResolver& drawable_resolver,
    const SkinFontResolver& font_resolver,
    SkinLoadReport& report,
    const SkinLoadLimits& limits = {}
) noexcept;

/**
 * @brief Composite file picker over one HoloDrive virtual namespace.
 *
 * The picker owns ordinary GUI child widgets. It borrows a HoloDrive for
 * sorted directory enumeration and an AssetManager for typed selected-file
 * loading and skin assets. Both services must outlive the picker. All methods
 * are single-thread confined with the borrowed services. No Lua binding exists.
 */
class FilePicker final : public Window {
public:
    /** @brief Completion callback invoked once by accept() or cancel(). */
    using ResultCallback = std::function<void(const FilePickerResult&)>;

    /**
     * @brief Construct and immediately enumerate a composite picker.
     * @param drive Borrowed virtual filesystem; must outlive this picker.
     * @param assets Borrowed typed cache bound to `drive`; must outlive this picker.
     * @param options Initial path, selection mode, title, and gdx-holo style names.
     * @param result Optional completion callback copied into the picker.
     * @post A failed initial listing leaves the picker visible with a diagnostic.
     */
    FilePicker(
        holodisk::HoloDrive& drive,
        holodisk::AssetManager& assets,
        FilePickerOptions options = {},
        ResultCallback result = {}
    );

    /** @brief Re-enumerate the current path without changing it on failure. */
    [[nodiscard]] holodisk::Status refresh();
    /** @brief Navigate transactionally to an absolute virtual directory. */
    [[nodiscard]] holodisk::Status navigate_to(std::string_view path);
    /** @brief Navigate to the parent directory, remaining at root when needed. */
    [[nodiscard]] holodisk::Status navigate_up();
    /** @brief Select a visible child by its single-segment name. */
    [[nodiscard]] holodisk::Status select_entry(std::string_view name);
    /** @brief Enter a visible directory, or select a visible file. */
    [[nodiscard]] holodisk::Status activate_entry(std::string_view name);
    /** @brief Deliver the current acceptable selection and request closure. */
    [[nodiscard]] bool accept();
    /** @brief Deliver a cancellation result and request closure. */
    void cancel();

    /** @brief Return the committed absolute virtual directory path. */
    [[nodiscard]] const std::string& current_path() const noexcept { return current_path_; }
    /** @brief Return the current sorted immediate-child snapshot. */
    [[nodiscard]] const std::vector<holodisk::Entry>& entries() const noexcept { return entries_; }
    /** @brief Return the selected visible entry, if any. */
    [[nodiscard]] const std::optional<holodisk::Entry>& selection() const noexcept { return selection_; }
    /** @brief Return the latest user-visible drive diagnostic, or empty. */
    [[nodiscard]] const std::string& last_error() const noexcept { return last_error_; }
    /** @brief Access the borrowed manager for application-specific loaders. */
    [[nodiscard]] holodisk::AssetManager& asset_manager() noexcept { return *assets_; }

    /**
     * @brief Load the selected file through the bound typed AssetManager.
     * @tparam T Asset type whose loader was registered with the manager.
     * @return Shared immutable asset, or InvalidArgument without a file selection.
     */
    template<typename T>
    [[nodiscard]] holodisk::Result<holodisk::AssetHandle<T>> load_selected()
    {
        if (!selection_ || selection_->directory) {
            return holodisk::Result<holodisk::AssetHandle<T>>::failure({
                holodisk::ErrorCode::InvalidArgument,
                "file picker has no selected file"
            });
        }
        return assets_->load<T>(selected_path());
    }

private:
    [[nodiscard]] std::string selected_path() const;
    [[nodiscard]] bool selection_allowed(bool directory) const noexcept;
    void rebuild_entry_widgets();
    void update_status();
    void report_error(const holodisk::Error& error);

    holodisk::HoloDrive* drive_;
    holodisk::AssetManager* assets_;
    FilePickerOptions options_;
    ResultCallback result_;
    std::string current_path_{"/"};
    std::vector<holodisk::Entry> entries_;
    std::optional<holodisk::Entry> selection_;
    std::string last_error_;
    bool completed_{false};
    TextField* path_field_{nullptr};
    ScrollPane* entries_pane_{nullptr};
    Label* status_label_{nullptr};
};

} // namespace squared::gui
