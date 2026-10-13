#pragma once

#include <squared/gui/check_box.hpp>

#include <string>

namespace sq::gui {

/** @brief CheckBox-shaped choice intended for a one-of-many ButtonGroup. */
class RadioButton final : public CheckBox {
public:
    /**
     * @brief Construct a radio choice using the named `radio` check style.
     * @param text Optional label text.
     * @param checked Initial checked state before group registration.
     */
    explicit RadioButton(std::string text = {}, bool checked = false);
};

} // namespace sq::gui
