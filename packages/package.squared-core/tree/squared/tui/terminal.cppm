export module sq.tui:terminal;

export import :event;
export import :input_decoder;
export import :renderer;
export import :screen;
import std;

export namespace sq::tui
{

/** @brief What a Terminal turns on besides raw input. */
struct TerminalOptions
{
        /** The alternate screen: the shell's screen comes back as it was on exit. */
        bool alternate_screen = true;
        /** Mouse reports (buttons, wheel and motion with a button held; SGR). */
        bool mouse = false;
        /** Pasted text as one Paste event (bracketed paste). */
        bool paste = true;
        /** Focus in and out events. */
        bool focus = false;
        /** The terminal's file descriptors: its input and its output. */
        int input = 0;
        int output = 1;
};

/** @brief Why a Terminal could not be opened. */
enum class TerminalError : std::uint8_t {
    /** The input is not a terminal (a pipe, a file). */
    not_a_terminal,
    /** Its mode could not be read or set. */
    no_mode,
    /** Another Terminal is open in this process. */
    already_open,
};

/** @brief What a TerminalError says, for a person. */
[[nodiscard]] std::string_view describe( TerminalError error ) noexcept;

/**
 * @brief The terminal a program runs in, held in raw mode while it lives.
 *
 * Opening it reads the terminal's mode, switches to raw input (every key as
 * it is typed, nothing echoed, Ctrl+C a key), the alternate screen and the
 * reports asked for. Closing it, by its destructor, puts all of that back.
 * So does a crash: a fatal signal (SIGSEGV, SIGABRT, SIGTERM, ...), an
 * exit() that skips destructors, or std::terminate; the shell is usable
 * after any of them. One at a time in a process.
 *
 * POSIX: Linux, Android (sqterm, Termux), the BSDs.
 */
class Terminal
{
    public:
        [[nodiscard]] static std::expected<Terminal, TerminalError>
        open( const TerminalOptions &options = {} );

        Terminal( Terminal &&other ) noexcept;
        Terminal &operator=( Terminal && ) = delete;
        Terminal( const Terminal & ) = delete;
        Terminal &operator=( const Terminal & ) = delete;
        ~Terminal();

        /** The size, in cells, as last told (a Resize event says when it changes). */
        [[nodiscard]] std::uint16_t columns() const noexcept { return columns_; }
        [[nodiscard]] std::uint16_t rows() const noexcept { return rows_; }

        /**
         * The next event, waiting at most `timeout` for one (forever when
         * negative); nothing when none came.
         */
        [[nodiscard]] std::optional<Event> next( std::chrono::milliseconds timeout );

        /** Shows `screen`: what changed since the last frame. */
        bool show( const Screen &screen );

        /** Bytes to the terminal as they are. */
        bool write( std::string_view bytes ) noexcept;

    private:
        Terminal() = default;
        bool read_size() noexcept;

        bool open_ = false;
        int input_ = 0;
        int output_ = 1;
        std::uint16_t columns_ = 80;
        std::uint16_t rows_ = 24;
        InputDecoder decoder_;
        std::vector<Event> queue_;
        std::size_t queue_front_ = 0;
        Renderer renderer_;
};

} // namespace sq::tui
