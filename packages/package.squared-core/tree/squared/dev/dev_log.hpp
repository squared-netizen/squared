#pragma once

#include <cstdint>
#include <string_view>

namespace sq::dev
{

/// Outcome of open_log_sink().
enum class LogSinkStatus : std::uint8_t {
    Opened,       ///< The sink is open and the crash handler is installed.
    AlreadyOpen,  ///< A sink is already open; nothing changed.
    SocketFailed, ///< The OS refused the socket. On Android this is almost
                  ///< always the missing INTERNET permission.
};

/// Where the sink sends and what it watches.
struct LogSinkOptions
{
        std::uint16_t port = 5555; ///< UDP port on 127.0.0.1.
        bool catch_crashes = true; ///< Report fatal signals before the process dies.
};

/**
 * @brief Send log lines, and crash reports, to a UDP port on this device.
 *
 * A development aid for working on a phone with no computer attached. Run a
 * listener in the same device's terminal, for example `nc -klu 5555`, and each
 * line arrives as it is written. The sink uses the loopback address only: no
 * network, no Wi-Fi, and no data leaves the device.
 *
 * Nothing here allocates after open_log_sink() returns, and nothing is
 * buffered, so the last line written before a crash is still delivered.
 *
 * @param options Port and crash-handler switch.
 * @return Opened on success; AlreadyOpen if called twice; SocketFailed if the
 * OS refused the socket.
 */
[[nodiscard]] LogSinkStatus open_log_sink( const LogSinkOptions &options = {} ) noexcept;

/**
 * @brief Send one line. Does nothing if no sink is open.
 * @param line Text without a trailing newline; longer than 1000 bytes is cut.
 * Never blocks and never fails visibly: a missing listener drops the line.
 */
void log_line( std::string_view line ) noexcept;

/// Close the sink and restore the signal handlers it replaced.
void close_log_sink() noexcept;

} // namespace sq::dev
