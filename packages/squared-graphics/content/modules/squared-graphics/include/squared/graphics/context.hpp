#pragma once

#include <squared/graphics/color.hpp>

struct SDL_Window;

namespace squared::graphics {

/**
 * @brief Own the SDL window and OpenGL ES context used by an application.
 *
 * The generated platform layer owns this object. Application rendering code
 * receives higher-level graphics objects and does not present the window
 * directly.
 */
class Context final {
public:
    Context() noexcept = default;
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;
    ~Context();

    /**
     * @brief Create an OpenGL ES 2.0 window and make its context current.
     *
     * @param title Window title.
     * @param logical_width Initial logical width.
     * @param logical_height Initial logical height.
     * @return `true` when the window and context are ready.
     */
    [[nodiscard]] bool create(
        const char* title,
        int logical_width,
        int logical_height
    ) noexcept;

    /** @brief Destroy the context and window. */
    void destroy() noexcept;

    /** @brief Refresh drawable dimensions and apply the OpenGL viewport. */
    void refresh_viewport() noexcept;

    /** @brief Clear the active color buffer. */
    void clear(Color color) noexcept;

    /** @brief Present the completed frame. */
    void present() noexcept;

    /** @brief Return whether a usable context exists. */
    [[nodiscard]] bool valid() const noexcept;

    /** @brief Return the current framebuffer width in pixels. */
    [[nodiscard]] int pixel_width() const noexcept;

    /** @brief Return the current framebuffer height in pixels. */
    [[nodiscard]] int pixel_height() const noexcept;

private:
    SDL_Window* window_{nullptr};
    void* native_context_{nullptr};
    int pixel_width_{0};
    int pixel_height_{0};
};

}  // namespace squared::graphics
