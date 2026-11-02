export module sq.gui:src.font_ptr;

export import :src.font_resource;
import std;

export namespace sq::gui
{

/** @brief Immutable shared ownership of one GUI font resource. */
using FontPtr = std::shared_ptr<const FontResource>;

} // namespace sq::gui
