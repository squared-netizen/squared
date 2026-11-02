export module sq.console:src.level;

import std;

export namespace sq::console
{

/** @brief How a console line is shown and filtered: `copy errors` keeps only `error`. */
enum class Level : std::uint8_t { info, warning, error };

} // namespace sq::console
