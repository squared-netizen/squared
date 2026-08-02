#pragma once

#include <squared/application/event.hpp>

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

    /**
     * @brief Initialize logical state once.
     *
     * @return `true` when the application may enter its event loop.
     */
    [[nodiscard]] virtual bool create(
        graphics::Context& graphics
    ) = 0;

    /** @brief Receive one platform-neutral input or lifecycle event. */
    virtual void handle_event(const Event& event) = 0;

    /** @brief Advance logical state once for the current frame. */
    virtual void update(std::chrono::nanoseconds delta) = 0;

    /** @brief Render one frame using the active graphics context. */
    virtual void render(graphics::Context& graphics) = 0;

    /** @brief Notify the application that frame updates are pausing. */
    virtual void pause() {}

    /** @brief Notify the application that frame updates are resuming. */
    virtual void resume() {}

    /** @brief Notify the application of current drawable dimensions. */
    virtual void resize(int, int) {}

    /** @brief Notify the application that a graphics surface is available. */
    virtual void surface_created(graphics::Context&) {}

    /** @brief Notify the application that its graphics surface is unavailable. */
    virtual void surface_destroyed() {}

    /** @brief Release logical and framework resources before destruction. */
    virtual void dispose() = 0;

    /** @brief Report whether application logic requested shutdown. */
    [[nodiscard]] virtual bool quit_requested() const noexcept = 0;
};

}  // namespace squared::application
