export module sq.term:terminal_config;

import std;

export namespace sq::term
{

/** @brief A terminal's size and the memory it keeps for lines scrolled off the top. */
struct TerminalConfig
{
        std::uint16_t columns = 80;
        std::uint16_t rows = 24;
        /**
         * Bytes for the scrollback (decided 2026-11-02: 1 MB, settable per
         * session): lines with their trailing blanks trimmed, 12 bytes a
         * cell and 24 a line, oldest out. 0 keeps none.
         */
        std::size_t scrollback_bytes = std::size_t{ 1 } << 20U;
};

} // namespace sq::term
