export module sq.messaging:snapshot_key;

export import :name;
import std;

export namespace sq::messaging
{

/**
 * @brief The name of a snapshot: bulk state the model publishes whole and
 * the UI reads the newest of (a terminal's screen, a canvas's pixels), the
 * third channel beside input and telegrams.
 *
 * ```cpp
 * inline constexpr sq::SnapshotKey screen{ "term.screen" };
 * ```
 *
 * A snapshot has no type the compiler checks: it is bytes, laid out by the
 * model and read by the actor that draws it, which agree on the layout (a
 * `term::Terminal` writes and reads its own, `term::write_snapshot`). 16
 * bytes: the name and its text, for the dev tools.
 */
class SnapshotKey final
{
    public:
        template<std::size_t Length>
        // NOLINTNEXTLINE(google-explicit-constructor): a literal is a key, as in "term.screen"
        consteval SnapshotKey( const char ( &text )[Length] ) noexcept
            : name_( text )
            , text_( text )
        {}

        [[nodiscard]] constexpr Name name() const noexcept { return name_; }
        [[nodiscard]] constexpr std::string_view text() const noexcept { return text_; }

    private:
        Name name_;
        const char *text_;
};

} // namespace sq::messaging
