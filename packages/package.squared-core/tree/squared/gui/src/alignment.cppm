export module sq.gui:src.alignment;

import sq.std;

export namespace sq::gui
{

/** @brief Per-cell alignment relative to the cell box. */
enum class Alignment : std::uint8_t { start, center, end };

} // namespace sq::gui
