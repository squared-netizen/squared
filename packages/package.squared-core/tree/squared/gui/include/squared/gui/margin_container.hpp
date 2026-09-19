#pragma once

#include <squared/gui/insets.hpp>
#include <squared/gui/size.hpp>
#include <squared/gui/widget.hpp>

#include <memory>

namespace sq::gui {

class Painter;
class Skin;

/** @brief Container adding fixed insets around one child. */
class MarginContainer final : public Widget {
public:
    /**
     * @brief Construct a margin container.
     * @param margin Insets in logical units; non-negative.
     */
    explicit MarginContainer(Insets margin = {}) noexcept;

    /**
     * @brief Set the single wrapped child.
     * @param content Widget to adopt; ownership transfers to the container.
     * @return Reference to the adopted content widget.
     */
    Widget& set_content(std::unique_ptr<Widget> content);

    /**
     * @brief Report the content size plus margins.
     * @param painter Active painter used for measurement.
     * @param skin Skin forwarded to the content widget.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Position the content inside the margin insets.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing style values.
     */
    void layout(Painter&, const Skin&) override;

private:
    Insets margin_;
    Widget* content_{nullptr};
};

} // namespace sq::gui
