#pragma once

#include <squared/gui/rectangle.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace sq::gui {

class Painter;
class Skin;
class Table;
struct PointerEvent;

/**
 * @brief Floating table-backed panel with a draggable title bar.
 *
 * Windows are hosted by Ui; ownership transfers to the Ui when shown.
 */
class Window : public Widget {
public:
    /**
     * @brief Construct a window.
     * @param title Title-bar text.
     * @param style Named window style to apply.
     */
    explicit Window(std::string title = {}, std::string style = "default");

    /**
     * @brief Access the window's content table.
     * @return Reference to the content table for adding children.
     */
    [[nodiscard]] Table& content_table() noexcept { return *content_; }

    /**
     * @brief Access the window's content table, read-only.
     * @return Reference to the content table.
     */
    [[nodiscard]] const Table& content_table() const noexcept { return *content_; }

    /**
     * @brief Replace the title-bar text.
     * @param title New title text.
     */
    void set_title(std::string title);

    /**
     * @brief Read the title text.
     * @return Reference to the stored title.
     */
    [[nodiscard]] const std::string& title() const noexcept { return title_; }

    /**
     * @brief Change the applied window style.
     * @param style Named window style to apply.
     */
    void set_style(std::string style);

    /**
     * @brief Control modal blocking.
     * @param modal true dims the background outside the window.
     */
    void set_modal(bool modal) noexcept { modal_ = modal; }

    /**
     * @brief Read the modal state.
     * @return true while the window blocks background interaction.
     */
    [[nodiscard]] bool modal() const noexcept { return modal_; }

    /**
     * @brief Control title-bar dragging.
     * @param movable false keeps the window at its placed position.
     */
    void set_movable(bool movable) noexcept { movable_ = movable; }

    /**
     * @brief Read the movable state.
     * @return true while the window can be dragged by its title bar.
     */
    [[nodiscard]] bool movable() const noexcept { return movable_; }

    /**
     * @brief Control the close button.
     * @param closable true shows the close button.
     */
    void set_closable(bool closable) noexcept;

    /**
     * @brief Read the closable state.
     * @return true while the window shows a close button.
     */
    [[nodiscard]] bool closable() const noexcept { return closable_; }

    /**
     * @brief Control edge resizing.
     * @param resizable true enables resize grabs on the window border.
     */
    void set_resizable(bool resizable) noexcept { resizable_ = resizable; }

    /**
     * @brief Read the resizable state.
     * @return true while the window can be resized.
     */
    [[nodiscard]] bool resizable() const noexcept { return resizable_; }

    /**
     * @brief Set the smallest allowed window size.
     * @param size Minimum extent in logical units.
     */
    void set_minimum_window_size(Size size) noexcept;

    /**
     * @brief Read the minimum window size.
     * @return Requested minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_window_size() const noexcept
    {
        return requested_minimum_;
    }

    /**
     * @brief Request the window to close.
     *
     * The host Ui prunes windows marked close-requested on the next update.
     */
    void request_close() noexcept { close_requested_ = true; }

    /**
     * @brief Read the close-requested flag.
     * @return true after request_close() was called.
     */
    [[nodiscard]] bool close_requested() const noexcept { return close_requested_; }

    /**
     * @brief Report the minimum size constrained by layout.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing the window style.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the content-driven preferred size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing window metrics and the title font.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Lay out the title bar and content table.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing the window style.
     */
    void layout(Painter&, const Skin&) override;

    /**
     * @brief Paint the title bar, background, and content.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the window style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

    /**
     * @brief Handle drag, resize, and close-button events.
     * @note The parameter matches Widget::pointer_event: a pointer payload in
     * widget-local coordinates.
     * @return true when the event was handled.
     */
    bool pointer_event(const PointerEvent&) override;

protected:
    /**
     * @brief Decide whether Escape closes the window.
     * @return false by default; overridden by Dialog.
     */
    [[nodiscard]] virtual bool escape_closes() const noexcept { return false; }

private:
    friend class Ui;
    enum ResizeEdge : unsigned int {
        resize_none = 0,
        resize_left = 1,
        resize_top = 2,
        resize_right = 4,
        resize_bottom = 8
    };
    [[nodiscard]] Rectangle close_bounds() const noexcept;
    [[nodiscard]] unsigned int resize_edges(float x, float y) const noexcept;
    void constrain_to_parent() noexcept;
    void resize_from_pointer(float stage_x, float stage_y) noexcept;
    std::string title_;
    std::string style_;
    Table* content_{nullptr};
    bool modal_{false};
    bool movable_{true};
    bool closable_{false};
    bool resizable_{false};
    bool close_requested_{false};
    bool close_hovered_{false};
    bool close_pressed_{false};
    float title_height_{36.0F};
    float close_size_{28.0F};
    float resize_border_{8.0F};
    Size requested_minimum_{};
    Size measured_minimum_{};
    std::optional<std::int64_t> interaction_pointer_;
    unsigned int resize_edges_{resize_none};
    float drag_offset_x_{0.0F};
    float drag_offset_y_{0.0F};
    Rectangle resize_start_{};
    float pointer_start_x_{0.0F};
    float pointer_start_y_{0.0F};
};

} // namespace sq::gui
