#pragma once

#include <squared/console/console.hpp>
#include <squared/messaging/name_table.hpp>
#include <squared/messaging/telegram_data.hpp>
#include <squared/messaging/telegraph.hpp>
#include <squared/time/time_point.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace sq::console
{

/**
 * @brief The Telegraph's development tools, on the UI's end of a line: its
 * Console commands, the name table its text form uses, the tap that writes
 * every telegram as a line, and recording and replaying what the UI sends.
 *
 * | Typed | Does |
 * |---|---|
 * | `telegraph` | each capacity and its highest fill since start |
 * | `telegraph state` | every value the UI holds, one line each, ordered by address |
 * | `telegraph tap [on \| off]` | every telegram as a line (`ui>model set settings.sound false`), or not; off at first |
 * | `telegraph record [file] \| record stop` | what the UI sends, each line with the milliseconds since the start, to `file` (`replay.txt`) in the files directory |
 * | `telegraph replay [file] [fast] \| replay stop` | send a recording again, at its pace or as fast as the ring takes it (`update` paces it) |
 *
 * For Console builds: about 15 KB inline (the name table, `TELEGRAPH_NAMES`,
 * most of it), nothing allocated. One to a line, on the UI thread; it must
 * outlive the line's use of it, and it detaches itself when it goes.
 */
class TelegraphTools final
{
    public:
        /** @brief Where the tap's lines go: the dev log in `DEV_LOG` builds. */
        using Sink = void ( * )( void *context, std::string_view line ) noexcept;

        /**
         * @brief The tools for `ui`; they keep its name table from now on.
         * @param directory Where `record` writes and `replay` reads: the
         * app's `files/` directory. nullptr: neither is offered.
         */
        TelegraphTools( messaging::TelegraphUi &ui, const char *directory ) noexcept;
        ~TelegraphTools();
        TelegraphTools( const TelegraphTools & ) = delete;
        TelegraphTools &operator=( const TelegraphTools & ) = delete;
        TelegraphTools( TelegraphTools && ) = delete;
        TelegraphTools &operator=( TelegraphTools && ) = delete;

        /** @brief `telegraph`, and the groups `telegraph` and `state`. How many did not fit. */
        [[nodiscard]] std::size_t add_commands( Console &console ) noexcept;

        /** @brief Where the tap's lines go; nullptr (the default): the Console's scrollback. */
        void set_sink( Sink sink, void *context ) noexcept;

        /** @brief Once a frame: the replay's next telegrams, as their time comes. */
        void update( time::TimePoint now ) noexcept;

        /** @brief Turn the tap on or off. */
        void tap( bool on ) noexcept;

        /** @brief Record to `file` in the directory; false when it cannot be opened. */
        [[nodiscard]] bool record( std::string_view file ) noexcept;
        /** @brief Stop recording; how many telegrams were written. */
        std::uint32_t stop_recording() noexcept;

        /** @brief Replay `file`, paced by `update` (or `fast`); false when it cannot be opened. */
        [[nodiscard]] bool replay( std::string_view file, bool fast ) noexcept;
        void stop_replay() noexcept;

        [[nodiscard]] const messaging::NameTable &names() const noexcept { return names_; }
        [[nodiscard]] const messaging::TelegraphUi &ui() const noexcept { return ui_; }
        [[nodiscard]] bool tapping() const noexcept { return tapping_; }
        [[nodiscard]] bool recording() const noexcept { return record_fd_ >= 0; }
        [[nodiscard]] bool replaying() const noexcept { return replay_fd_ >= 0; }

    private:
        static void tapped( void *context, messaging::TapDirection direction,
                            const messaging::Telegram &telegram,
                            std::span<const std::byte> data ) noexcept;
        void attach_tap() noexcept;
        void say( Level level, std::string_view line ) noexcept;
        [[nodiscard]] bool path_of( std::string_view file ) noexcept;
        [[nodiscard]] bool next_line( std::string_view &line ) noexcept;
        void finish_replay() noexcept;

        messaging::TelegraphUi &ui_;
        const char *directory_;
        Console *console_ = nullptr;
        Sink sink_ = nullptr;
        void *sink_context_ = nullptr;

        std::array<std::byte, messaging::telegraph_size::names> name_storage_{};
        messaging::NameTable names_;
        std::array<char, format_bytes> line_{};
        std::array<char, 512> path_{};
        bool tapping_ = false;

        // Recording: a file, and when it started.
        int record_fd_ = -1;
        time::TimePoint record_start_{};
        std::uint32_t recorded_ = 0;

        // Replaying: a file read through a buffer, a line at a time, and the
        // telegram read from the last line, until its time comes.
        int replay_fd_ = -1;
        bool replay_fast_ = false;
        bool replay_started_ = false;
        bool replay_end_ = false;
        time::TimePoint replay_start_{};
        std::int64_t pending_due_ms_ = 0;
        std::optional<messaging::Telegram> pending_;
        std::array<char, 4096> read_{};
        std::size_t read_start_ = 0;
        std::size_t read_end_ = 0;
        std::uint32_t replay_line_ = 0;
        std::uint32_t replayed_ = 0;
        std::uint32_t unread_ = 0;
        std::array<std::byte, messaging::telegraph_size::data / 4> value_storage_{};
        messaging::TelegramData value_;
};

} // namespace sq::console
