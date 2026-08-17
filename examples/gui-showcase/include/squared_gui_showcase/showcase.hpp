#pragma once

#include <squared/gui/gui.hpp>

#include <memory>
#include <cstdint>
#include <string>
#include <string_view>

namespace squared_gui_showcase {

/** Builds and owns the portable widget tree used by the visual test app. */
class Showcase final {
public:
    /**
     * @brief Construct the showcase widget tree.
     * @param width Initial widget-space width in logical pixels.
     * @param height Initial widget-space height in logical pixels.
     * @param skin Shared skin styling the created widgets; must remain valid
     * for the lifetime of the Ui.
     * @param image Optional image drawable used by the launcher; may be null.
     */
    Showcase(
        float width,
        float height,
        squared::gui::Skin skin,
        squared::gui::DrawablePtr image = {}
    );

    /**
     * @brief Access the mutable user-interface root.
     * @return Reference to the owned Ui; stable until destruction.
     */
    [[nodiscard]] squared::gui::Ui& ui() noexcept { return ui_; }

    /**
     * @brief Access the read-only user-interface root.
     * @return Reference to the owned Ui; stable until destruction.
     */
    [[nodiscard]] const squared::gui::Ui& ui() const noexcept { return ui_; }

    /**
     * @brief Read the current status line.
     * @return Text currently shown in the status label.
     */
    [[nodiscard]] std::string_view status() const noexcept { return status_; }

    /** @brief Show the controls demo window. */
    void show_controls_window();

    /** @brief Show the layout demo window. */
    void show_layout_window();

    /** @brief Show the widget-inspector window. */
    void show_inspector_window();

    /** @brief Show the confirmation-dialog demo. */
    void show_confirmation_dialog();

    /** @brief Show the about dialog. */
    void show_about_dialog();

    /** @brief Recompute the status line from current widget state. */
    void refresh_status();

    /**
     * @brief Incorporate a graphics-context recovery outcome.
     * @param generation Monotonic graphics recovery generation.
     * @param resources_preserved Whether texture resources survived recovery.
     */
    void report_graphics_recovery(
        std::uint64_t generation,
        bool resources_preserved
    );

private:
    void set_status(std::string status);
    std::unique_ptr<squared::gui::Widget> make_launcher();
    std::unique_ptr<squared::gui::Window> make_controls_window();
    std::unique_ptr<squared::gui::Window> make_layout_window();
    std::unique_ptr<squared::gui::Window> make_inspector_window();

    squared::gui::Ui ui_;
    squared::gui::DrawablePtr image_;
    squared::gui::Label* status_label_{nullptr};
    squared::gui::Label* volume_label_{nullptr};
    squared::gui::TextField* name_field_{nullptr};
    std::string status_{"Ready"};
};

} // namespace squared_gui_showcase
