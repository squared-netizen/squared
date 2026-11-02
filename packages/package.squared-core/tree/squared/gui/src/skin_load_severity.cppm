export module sq.gui:src.skin_load_severity;

import std;

export namespace sq::gui
{

/**
 * @brief Severity of one skin-loading diagnostics entry.
 */
enum class SkinLoadSeverity : std::uint8_t { warning, error };

} // namespace sq::gui
