#pragma once

#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

#include <cstdint>
#include <memory>
#include <optional>

namespace sq::gui {

class Painter;
class Skin;
struct PointerEvent;

/** @brief Container clipping and vertically scrolling a larger child. */
class ScrollPane final : public Widget {
public:
    /**
     * @brief Set the single scrollable child.
     * @param content Widget to adopt; ownership transfers to the pane.
     * @return Reference to the adopted content widget.
     */
    Widget& set_content(std::unique_ptr<Widget> content);

    /**
     * @brief Set the vertical scroll offset.
     * @param scroll_y Offset in logical units; clamped to valid range.
     */
    void set_scroll_y(float scroll_y) noexcept;

    /**
     * @brief Read the vertical scroll offset.
     * @return Current offset in logical units.
     */
    [[nodiscard]] float scroll_y() const noexcept { return scroll_y_; }

    /**
     * @brief Report the content size.
     * @param painter Active painter used for measurement.
     * @param skin Skin forwarded to the content widget.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Position the content and clamp the scroll offset.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing style values.
     */
    void layout(Painter&, const Skin&) override;

    /**
     * @brief Handle drag-to-scroll pointer events.
     * @note The parameter matches Widget::pointer_event: a pointer payload in
     * widget-local coordinates.
     * @return true when the event was handled.
     */
    bool pointer_event(const PointerEvent&) override;

private:
    void clamp_scroll() noexcept;
    Widget* content_{nullptr};
    float scroll_y_{0.0F};
    float last_pointer_y_{0.0F};
    std::optional<std::int64_t> drag_pointer_;
};

} // namespace sq::gui
