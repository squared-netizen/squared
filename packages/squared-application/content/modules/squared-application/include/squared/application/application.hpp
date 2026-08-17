#pragma once

#include <squared/application/event.hpp>
#include <squared/application/text_input.hpp>

#include <chrono>

namespace squared::graphics {
class Context;
}

namespace squared::application {

/**
 * @brief Developer-owned application behind a platform-neutral lifecycle.
 *
 * The generated SDL adapter owns process and platform setup. Developer code
 * implements this interface and uses Squared framework services for ordinary
 * application behavior.
 */
class Application {
public:
    virtual ~Application() = default;

    /** Supply the portable application-to-platform text-input service. */
    virtual void set_text_input_service(TextInputService*) noexcept {}

    /**
     * @brief Initialize logical state once.
     *
     * @param graphics The active platform rendering context.
     * @return `true` when the application may enter its event loop.
     */
    [[nodiscard]] virtual bool create(
        graphics::Context& graphics
    ) = 0;

    /**
     * @brief Receive one platform-neutral input or lifecycle event.
     * @param event Portable event produced by the platform adapter.
     */
    virtual void handle_event(const Event& event) = 0;

    /**
     * @brief Advance logical state once for the current frame.
     * @param delta Elapsed domain time since the previous update, in
     * nanoseconds.
     */
    virtual void update(std::chrono::nanoseconds delta) = 0;

    /**
     * @brief Render one frame using the active graphics context.
     * @param graphics The active platform rendering context.
     */
    virtual void render(graphics::Context& graphics) = 0;

    /** @brief Notify the application that frame updates are pausing. */
    virtual void pause() {}

    /** @brief Notify the application that frame updates are resuming. */
    virtual void resume() {}

    /** @brief Notify the application of current drawable dimensions. */
    virtual void resize(int, int) {}

    /** @brief Restore or validate GPU resources after a surface becomes usable. */
    virtual void surface_created(graphics::Context&) {}

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

}  // namespace squared::application
