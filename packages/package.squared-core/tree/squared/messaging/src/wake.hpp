#pragma once

namespace sq::messaging
{

/**
 * @brief One thread sleeps on it, another wakes it: an `eventfd`, so the
 * sleeper can wait on other file descriptors in the same `poll` (a pty, a
 * child's pipes), and Android's looper can watch it.
 *
 * Signalled only when the sleeper says it is asleep (a flag beside it, the
 * Telegraph's), so a busy thread costs no system call.
 */
class Wake final
{
    public:
        Wake() noexcept;
        ~Wake();
        Wake( const Wake & ) = delete;
        Wake &operator=( const Wake & ) = delete;
        Wake( Wake && ) = delete;
        Wake &operator=( Wake && ) = delete;

        /** @brief The descriptor to poll; -1 if it could not be made (then waits only time out). */
        [[nodiscard]] int fd() const noexcept { return fd_; }

        /** @brief Wake the sleeper (or its next sleep). */
        void signal() const noexcept;

        /** @brief Forget any signal: after waking, before looking for work. */
        void clear() const noexcept;

        /**
         * @brief Sleep until signalled or `timeout_ms` passes (-1: no
         * timeout). Returns at once when already signalled.
         */
        void wait( int timeout_ms ) const noexcept;

    private:
        int fd_ = -1;
};

} // namespace sq::messaging
