export module sq.tui:input_decoder;

export import :event;
import sq.term;
import std;

export namespace sq::tui
{

/**
 * @brief Bytes a terminal sends, into events: keys (UTF-8 text, control
 * keys, xterm's CSI and SS3 sequences with their modifiers, Alt as an
 * ESC prefix), SGR mouse reports, bracketed paste, focus.
 *
 * For keys, the inverse of sq.term's `key_bytes` and `key_text_bytes`
 * (what sqterm sends for a key): every key and modifier they encode
 * decodes back to itself, which the tests check.
 *
 * Fed as the bytes arrive, a sequence may be split between two reads. An
 * escape on its own cannot be told from the start of a sequence until
 * nothing follows it: `flush` says so, and the Terminal calls it when no
 * byte has come for a moment. Sequences it does not know are dropped
 * whole.
 */
class InputDecoder
{
    public:
        /** Decodes `bytes`, appending each whole event to `out`. */
        void feed( std::span<const std::byte> bytes, std::vector<Event> &out );
        void feed( std::string_view text, std::vector<Event> &out )
        {
            feed( std::as_bytes( std::span{ text } ), out );
        }

        /** Nothing more is coming for now: a lone escape is the Escape key. */
        void flush( std::vector<Event> &out );

        /** Bytes are held that may be a lone escape: worth a `flush` soon. */
        [[nodiscard]] bool waiting() const noexcept { return !pending_.empty() && !in_paste_; }

    private:
        /** One event from the front of `pending_`: bytes used, 0 when more are needed. */
        std::size_t decode( std::span<const std::uint8_t> bytes, std::vector<Event> &out );
        std::size_t decode_csi( std::span<const std::uint8_t> bytes, std::vector<Event> &out );
        std::size_t paste_bytes( std::vector<Event> &out );

        std::vector<std::uint8_t> pending_;
        bool in_paste_ = false;
        std::string paste_;
};

} // namespace sq::tui
