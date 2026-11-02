export module sq.gui:src.drawable_ptr;

export import :src.drawable;
import sq.std;

export namespace sq::gui
{

/**
 * @brief Shared ownership of an immutable drawable.
 */
using DrawablePtr = std::shared_ptr<const Drawable>;

} // namespace sq::gui
