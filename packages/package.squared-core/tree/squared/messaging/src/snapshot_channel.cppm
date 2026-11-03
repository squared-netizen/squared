export module sq.messaging:src.snapshot_channel;

export import :snapshot_view;
import std;

export namespace sq::messaging::detail
{

/**
 * Three buffers between one writer (the model's thread) and one reader (the
 * UI's): the writer fills its own, then trades it for the middle one; the
 * reader, when the middle one is newer than its own, trades its own for it.
 * Neither ever waits, and the reader always gets the newest whole snapshot:
 * a publish the reader never saw is simply replaced.
 *
 * Each buffer belongs to one side at a time, so the writer may make its own
 * larger before filling it (a terminal after a rotation), and only the
 * writer ever allocates. The middle buffer's index and whether it is newer
 * than the reader's are one atomic byte.
 */
class SnapshotChannel final
{
    public:
        SnapshotChannel() = default;
        SnapshotChannel( const SnapshotChannel & ) = delete;
        SnapshotChannel &operator=( const SnapshotChannel & ) = delete;
        SnapshotChannel( SnapshotChannel && ) = delete;
        SnapshotChannel &operator=( SnapshotChannel && ) = delete;
        ~SnapshotChannel() = default;

        /** The writer's buffer, at least `bytes` long: allocated only past its size. */
        [[nodiscard]] std::span<std::byte> back( std::size_t bytes )
        {
            Buffer &buffer = buffers_[back_];
            if( buffer.capacity < bytes ) {
                buffer.data = std::make_unique_for_overwrite<std::byte[]>( bytes );
                buffer.capacity = bytes;
            }
            return { buffer.data.get(), bytes };
        }

        /** The writer's first `bytes` are the newest: traded for the middle buffer. */
        void publish( std::size_t bytes ) noexcept
        {
            Buffer &buffer = buffers_[back_];
            buffer.size = std::min( bytes, buffer.capacity );
            buffer.sequence = ++published_;
            back_ = static_cast<std::uint8_t>(
                middle_.exchange( static_cast<std::uint8_t>( back_ | fresh ),
                                  std::memory_order_acq_rel ) &
                index_mask );
        }

        /** The reader's newest: its own, or the middle one when that is newer. */
        [[nodiscard]] std::optional<SnapshotView> latest() noexcept
        {
            if( ( middle_.load( std::memory_order_relaxed ) & fresh ) != 0 ) {
                front_ = static_cast<std::uint8_t>(
                    middle_.exchange( front_, std::memory_order_acq_rel ) & index_mask );
            }
            const Buffer &buffer = buffers_[front_];
            if( buffer.sequence == 0 ) {
                return std::nullopt;
            }
            return SnapshotView{ { buffer.data.get(), buffer.size }, buffer.sequence };
        }

    private:
        struct Buffer
        {
                std::unique_ptr<std::byte[]> data;
                std::size_t capacity = 0;
                std::size_t size = 0;
                std::uint64_t sequence = 0;
        };

        static constexpr std::uint8_t index_mask = 3;
        static constexpr std::uint8_t fresh = 4;

        std::array<Buffer, 3> buffers_{};
        std::atomic<std::uint8_t> middle_{ 1 };
        std::uint64_t published_ = 0; // the writer's
        std::uint8_t back_ = 0;       // the writer's
        std::uint8_t front_ = 2;      // the reader's
};

} // namespace sq::messaging::detail
