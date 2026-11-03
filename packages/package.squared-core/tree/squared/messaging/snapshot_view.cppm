export module sq.messaging:snapshot_view;

import std;

export namespace sq::messaging
{

/**
 * @brief The newest snapshot the UI has taken: its bytes, and which publish
 * it was (1 for the first, counting every publish, those the UI never saw
 * among them).
 */
struct SnapshotView
{
        std::span<const std::byte> bytes;
        std::uint64_t sequence = 0;
};

} // namespace sq::messaging
