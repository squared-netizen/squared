export module sq.term:pty;

export import :process_exit;
export import :pty_config;
export import :pty_error;
import std;

export namespace sq::term
{

/**
 * @brief A program running in a pseudo-terminal: what it writes is read
 * here, and fed to a `Terminal`; keys are written here.
 *
 * POSIX, the same on Linux and Android: `posix_openpt`, a new session for
 * the child with the pseudo-terminal as its controlling terminal, `execve`.
 * The terminal starts with UTF-8 input (IUTF8) and without XON/XOFF flow
 * control (Ctrl-S and Ctrl-Q reach the program), as Termux's.
 *
 * Reads never block (`read` returns 0 when nothing waits); the owner waits
 * on `fd()` with poll, or `wait_readable`. A write waits for room, up to its
 * timeout. Destroying it, or `hang_up`, ends the program: SIGHUP to its
 * process group, then SIGKILL if it is still there, and the process reaped.
 *
 * 16 bytes, and nothing allocated but by `spawn` (the program's arguments,
 * freed before it returns).
 *
 * @code
 * sq::term::Pty pty;
 * if( const auto error = pty.spawn( { .program = "/system/bin/sh" } ) ) {
 *     log( error.what() );
 * }
 * pty.write( "ls\r" );
 * std::array<std::byte, 4096> buffer;
 * terminal.feed( { buffer.data(), pty.read( buffer ) } );
 * @endcode
 */
class Pty final
{
    public:
        Pty() = default;
        Pty( const Pty & ) = delete;
        Pty &operator=( const Pty & ) = delete;
        Pty( Pty &&other ) noexcept;
        Pty &operator=( Pty &&other ) noexcept;
        ~Pty();

        /** Starts `config.program`; an error (true) when it could not. */
        [[nodiscard]] PtyError spawn( const PtyConfig &config );

        /** What the program wrote, up to `into.size()` bytes; 0 when nothing waits (or it is closed). */
        [[nodiscard]] std::size_t read( std::span<std::byte> into ) noexcept;

        /**
         * Bytes for the program (keys, a paste). Waits for room up to
         * `timeout_ms` in all; returns how many were written.
         */
        std::size_t write( std::span<const std::byte> bytes, int timeout_ms = 1000 ) noexcept;
        std::size_t write( std::string_view text, int timeout_ms = 1000 ) noexcept
        {
            return write( std::as_bytes( std::span{ text } ), timeout_ms );
        }

        /** The terminal's new size: the program gets SIGWINCH. */
        void resize( std::uint16_t columns, std::uint16_t rows ) noexcept;

        /** Waits up to `timeout_ms` for something to read (or for the end); true when there is. */
        [[nodiscard]] bool wait_readable( int timeout_ms ) const noexcept;

        /**
         * How the program ended, once it has (the process is then reaped);
         * nothing while it runs or when none was started.
         */
        [[nodiscard]] std::optional<ProcessExit> poll_exit() noexcept;

        /** No program will write any more: every process with the terminal open has closed it. */
        [[nodiscard]] bool closed() const noexcept { return closed_; }

        /** Ends the program (SIGHUP, then SIGKILL) and closes the terminal. */
        void hang_up() noexcept;

        /** The pseudo-terminal, to wait on with poll; -1 when none. */
        [[nodiscard]] int fd() const noexcept { return master_; }
        /** The program's process (its process group too); 0 when none. */
        [[nodiscard]] int pid() const noexcept { return pid_; }

    private:
        int master_ = -1;
        int pid_ = 0;
        int status_ = 0;
        bool exited_ = false;
        bool closed_ = false;
};

static_assert( sizeof( Pty ) <= 16 );

} // namespace sq::term
