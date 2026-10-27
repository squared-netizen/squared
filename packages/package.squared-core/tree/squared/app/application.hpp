#pragma once

#include <squared/app/event.hpp>
#include <squared/app/runtime.hpp>
#include <squared/app/safe_area.hpp>
#include <squared/app/src/report.hpp>
#include <squared/app/src/text_input.hpp>
#include <squared/app/src/text_input_service.hpp>

#include <chrono>
#include <cstddef>
#include <span>

namespace sq::graphics
{
class Context;
} // namespace sq::graphics

namespace sq::app
{

/**
 * @brief Developer-owned application behind a platform-neutral lifecycle.
 *
 * The generated SDL adapter owns process and platform setup. Developer code
 * implements this interface and uses Squared framework services for ordinary
 * application behavior.
 */
class Application
{
    public:
        virtual ~Application() = default;

        /** Supply the portable application-to-platform text-input service. */
        virtual void set_text_input_service( TextInputService * ) noexcept {}

        /**
         * @brief Where to say what went wrong. Given before create(), and
         * again whenever the platform changes it.
         */
        virtual void set_report( Report report ) noexcept { static_cast<void>( report ); }

        /**
     * @brief Initialize logical state once.
     *
     * @param runtime The platform services: rendering context, file system
     * and asset manager. Keep references to the members you use; the objects
     * they refer to live for the whole process.
     * @return `true` when the application may enter its event loop.
     * @note Only create() receives the whole runtime. render() still takes the
     * context alone, because a frame should not be reaching into the file
     * system or the asset cache sixty times a second.
     */
        [[nodiscard]] virtual bool create( Runtime &runtime ) = 0;

        /**
     * @brief Receive one platform-neutral input or lifecycle event.
     * @param event Portable event produced by the platform adapter.
     */
        virtual void handle_event( const Event &event ) = 0;

        /**
     * @brief Advance logical state once for the current frame.
     * @param delta Elapsed domain time since the previous update, in
     * nanoseconds.
     */
        virtual void update( std::chrono::nanoseconds delta ) = 0;

        /**
     * @brief Render one frame using the active graphics context.
     * @param graphics The active platform rendering context.
     */
        virtual void render( graphics::Context &graphics ) = 0;

        /** @brief Notify the application that frame updates are pausing. */
        virtual void pause() {}

        /** @brief Notify the application that frame updates are resuming. */
        virtual void resume() {}

        /**
         * @brief The application is visible again (Android's start). Not
         * called before create(); the first start precedes it.
         */
        virtual void foreground() {}

        /**
         * @brief The application is no longer visible (Android's stop): it may
         * never come back, so this is where state is made safe.
         */
        virtual void background() {}

        /** @brief The most `save_state` may write: what the platform keeps for it. */
        static constexpr std::size_t saved_state_bytes = 8192;

        /**
         * @brief What the platform kept for this activity, given before each
         * create(): on Android, its saved instance state. Android keeps it only
         * when it ended the process itself (memory, the freezer) or made the
         * activity again; a swipe from the recent apps, a finish, a force stop
         * or a reboot leaves it empty: a new launch. Valid until create()
         * returns.
         */
        virtual void set_saved_state( std::span<const std::byte> saved ) noexcept
        {
            static_cast<void>( saved );
        }

        /**
         * @brief Write what should come back with the activity into `out`
         * (Android's save instance state); the bytes written, 0 for nothing.
         */
        [[nodiscard]] virtual std::size_t save_state( std::span<std::byte> out ) noexcept
        {
            static_cast<void>( out );
            return 0;
        }

        /**
         * @brief Whether the application asked to go to the background (back
         * at its root screen): true once for each time it asked. The platform
         * then moves its task back, as Home does.
         */
        [[nodiscard]] virtual bool take_background_request() noexcept { return false; }

        /** @brief Whether the system bars should be hidden (immersive): read on each frame. */
        [[nodiscard]] virtual bool hides_bars() const noexcept { return false; }

        /** @brief The system is short of memory: drop what can be made again. */
        virtual void low_memory() {}

        /** @brief Notify the application of current drawable dimensions. */
        virtual void resize( int, int ) {}

        /**
     * @brief Tell the application which edges of the drawable are covered.
     *
     * Called when the platform first measures the bars and again whenever they
     * change (rotation, a hidden navigation bar). Not called on a platform
     * with nothing covering the window, so the default - ignore it - is right
     * for a desktop application.
     *
     * @param area Pixels covered along each edge of the drawable.
     */
        virtual void safe_area_changed( const SafeArea &area ) { static_cast<void>( area ); }

        /** @brief Restore or validate GPU resources after a surface becomes usable. */
        virtual void surface_created( graphics::Context & ) {}

        /**
     * @brief Mark GPU resources stale after rendering becomes unavailable.
     *
     * Implementations must not assume graphics commands are legal here.
     */
        virtual void surface_destroyed() {}

        /** @brief Release logical and framework resources before destruction. */
        virtual void dispose() = 0;

        /**
     * @brief Report whether application logic requested shutdown.
     * @return `true` when the platform should end the event loop.
     */
        [[nodiscard]] virtual bool quit_requested() const noexcept = 0;
};

} // namespace sq::app
