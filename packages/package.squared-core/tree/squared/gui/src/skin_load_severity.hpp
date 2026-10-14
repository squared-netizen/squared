#pragma once

#include <cstdint>

namespace sq::gui
{

/**
 * @brief Severity of one skin-loading diagnostics entry.
 */
enum class SkinLoadSeverity : std::uint8_t { warning, error };

} // namespace sq::gui
