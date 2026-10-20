#pragma once

#include <squared/console/src/bridge_protocol.hpp>
#include <squared/console/src/level.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sq::console
{

/** @brief Where the bridge listens, and what a client must say first. */
struct BridgeConfig
{
        /// The port on 127.0.0.1, reachable only from this device. 0 picks a free one.
        std::uint16_t port{ 0 };
        /// 16 to 64 letters and digits; a client that does not send it is closed.
        std::string_view secret;
        /// Send one UDP datagram to the same port number at open, so a client
        /// waiting for the app (Termux's `sqconsole`) knows to connect.
        bool announce{ true };
        /// Where a file the client pushes is written (copied, at most 255
        /// bytes); empty refuses pushes.
        std::string_view push_directory;
};

/** @brief What open() did. */
enum class BridgeStatus : std::uint8_t {
    listening,    ///< Open; port() says where.
    already_open, ///< Nothing changed.
    bad_secret,   ///< Not 16 to 64 letters and digits.
    refused,      ///< The system said no: the port in use, or no INTERNET permission on Android.
};

/** @brief Watch `fd` for reading and, when `write`, writing; both false: stop watching it. */
using BridgeWatch = void ( * )( void *context, int fd, bool read, bool write ) noexcept;
/** @brief A command line from the client, without its newline. */
using BridgeCommand = void ( * )( void *context, std::string_view line ) noexcept;
/** @brief A client gave the secret; `replay`: it wants the scrollback so far. */
using BridgeConnected = void ( * )( void *context, bool replay ) noexcept;
/** @brief What the bridge has to say about a transfer, for the Console (it is sent on to the client from there). */
using BridgeNote = void ( * )( void *context, Level level, std::string_view line ) noexcept;

/** @brief How the bridge reaches its owner: the event loop, and the Console. */
struct BridgeHandlers
{
        BridgeWatch watch{ nullptr };
        BridgeCommand command{ nullptr };
        BridgeConnected connected{ nullptr };
        /// Optional: without it, notes go to the client only.
        BridgeNote note{ nullptr };
        void *context{ nullptr };
};

/**
 * @brief The Console, reachable from Termux on the same device: every line it
 * prints goes out, and every line that comes in is run as a command.
 *
 * A TCP port on 127.0.0.1, so nothing off the device can reach it, and a
 * secret, so another app on it cannot either without reading this app's APK,
 * which holds the secret. Dev builds only (the Android template opens it in
 * CONSOLE builds). One client at a time: a newer one that gives the secret
 * takes over.
 *
 * It waits on events, never on a timer: open() asks the owner, through
 * `watch`, to tell it when a socket is ready, and ready() is how the owner
 * does (the template uses Android's looper). Nothing allocates. Output waits in
 * a fixed 16 KB buffer; when the client reads slower than lines arrive, lines
 * are dropped and counted, and the client is told how many once there is room.
 *
 * Files travel both ways, one at a time each way, in 4 KB chunks read and
 * written as the socket is ready, so a large one neither fills memory nor
 * stalls the frame: send_file() sends one (the `pull` command,
 * file_commands.hpp), and a client's `push` is written into
 * BridgeConfig::push_directory.
 *
 * ```cpp
 * sq::console::Bridge bridge;
 * bridge.open( { .port = 30123, .secret = secret }, { .watch = watch,
 *              .command = run_command, .connected = replay, .context = this } );
 * console.set_mirror( []( void *b, sq::console::Level level, std::string_view line ) noexcept {
 *     static_cast<sq::console::Bridge *>( b )->send( level, line ); }, &bridge );
 * ```
 *
 * Wire format: console/src/bridge_protocol.hpp. Not thread-safe: one thread.
 */
class Bridge final
{
    public:
        /// Output waiting for the client, at most.
        static constexpr std::size_t output_bytes = 16384;
        /// A command line from the client, at most; longer is cut.
        static constexpr std::size_t command_bytes = 256;
        /// A line sent to the client, at most; longer is cut.
        static constexpr std::size_t line_bytes = 1024;
        /// send_file()'s size when it is not known ahead (a /proc file says 0):
        /// sent until the file ends.
        static constexpr std::uint64_t unknown_size = ~std::uint64_t{ 0 };

        Bridge() noexcept = default;
        ~Bridge() { close(); }
        Bridge( const Bridge & ) = delete;
        Bridge &operator=( const Bridge & ) = delete;
        Bridge( Bridge && ) = delete;
        Bridge &operator=( Bridge && ) = delete;

        /** @brief Listen, and announce it. The secret is copied. */
        [[nodiscard]] BridgeStatus open( const BridgeConfig &config,
                                         const BridgeHandlers &handlers ) noexcept;

        /** @brief Close every socket, telling `watch` to stop watching each. */
        void close() noexcept;

        /**
         * @brief A socket `watch` named is ready.
         * @return false when that socket is closed now (its watch already stopped).
         */
        bool ready( int fd, bool readable, bool writable ) noexcept;

        /** @brief Send a line to the client, if one is connected; else nothing. */
        void send( Level level, std::string_view line ) noexcept;

        /**
         * @brief Send the open file `fd`, `size` bytes (or unknown_size), to
         * the client as `name` (a plain name, no '/'). The bridge reads it as the socket
         * takes it, and closes it. Called by a command, the command's end mark
         * follows the file's last byte.
         * @return false, leaving `fd` open, when no client is connected, a
         * file is already on its way, or `name` is not a plain name.
         */
        [[nodiscard]] bool send_file( int fd, std::string_view name, std::uint64_t size ) noexcept;

        /** @brief A file is being sent (send_file) or received (a push). */
        [[nodiscard]] bool transferring() const noexcept { return file_ >= 0 || push_left_ != 0; }

        [[nodiscard]] bool listening() const noexcept { return listener_ >= 0; }
        [[nodiscard]] bool connected() const noexcept { return client_ >= 0; }
        [[nodiscard]] std::uint16_t port() const noexcept { return port_; }
        /** @brief Lines dropped because the client read too slowly, since open(). */
        [[nodiscard]] std::uint64_t dropped() const noexcept { return dropped_; }

    private:
        void accept_clients() noexcept;
        bool read_hello() noexcept;
        bool read_commands() noexcept;
        bool consume( std::string_view bytes ) noexcept;
        bool run_line( std::string_view line ) noexcept;
        void start_push( std::string_view request ) noexcept;
        void finish_push() noexcept;
        void end_transfers() noexcept;
        void abandon_push() noexcept;
        bool pump_file() noexcept;
        void note( Level level, std::string_view line ) noexcept;
        void take_over() noexcept;
        void close_socket( int &fd ) noexcept;
        bool queue( char type, std::string_view text ) noexcept;
        bool append( char type, std::string_view text ) noexcept;
        void append_bytes( std::string_view bytes ) noexcept;
        void flush() noexcept;
        void watch_client() noexcept;

        std::array<char, output_bytes> output_{};
        std::array<char, command_bytes> command_{};
        std::array<char, 160> hello_{};
        std::array<char, 64> secret_{};
        std::array<char, 256> push_directory_{};
        std::array<char, bridge_protocol::name_max + 1> push_name_{};
        BridgeHandlers handlers_{};
        std::uint64_t dropped_{ 0 };
        std::uint64_t unreported_{ 0 }; // dropped since the client was last told
        std::size_t output_head_{ 0 };
        std::size_t output_size_{ 0 };
        std::size_t command_size_{ 0 };
        std::size_t hello_size_{ 0 };
        std::size_t secret_size_{ 0 };
        std::uint64_t file_left_{ 0 }; // bytes of the file being sent still to read
        std::uint64_t push_left_{ 0 }; // bytes of the push still to arrive
        std::uint64_t push_size_{ 0 };
        int listener_{ -1 };
        int client_{ -1 };  // gave the secret
        int pending_{ -1 }; // connected, not yet given it
        int file_{ -1 };    // the file being sent
        int push_{ -1 };    // the file being received; -1 while one refused is skipped
        std::uint16_t port_{ 0 };
        bool writing_{ false };         // the client's watch includes writing
        bool cutting_{ false };         // the rest of an over-long command is dropped
        bool done_after_file_{ false }; // the command that sent it is still to be marked done
};

static_assert( sizeof( Bridge ) <= std::size_t{ 17 } * 1024 + 512,
               "Bridge: see docs/developer/console.md" );

} // namespace sq::console
