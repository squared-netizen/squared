export module sq.console:src.line;

export import :src.level;
import std;

export namespace sq::console
{

/**
 * @brief One line of console text, as the scrollback hands it out.
 *
 * `text` points into the scrollback's storage: it is valid until the next
 * push() or clear(), which may overwrite it. Copy it out to keep it.
 */
struct Line final
{
        std::string_view text;
        Level level{ Level::info };
};

} // namespace sq::console
