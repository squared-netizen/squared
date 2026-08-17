#pragma once

#include <squared/graphics/color.hpp>

#include <cstdint>

namespace squared::graphics {

/**
 * @brief Own the native window and rendering context selected at link time.
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
     * @brief Create a platform window and make its rendering context current.
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

    /**
     * @brief Release the native rendering context while retaining the window.
     *
     * Callers may release GPU objects before this call while the context is
     * current. Android lifecycle handling normally lets SDL suspend EGL and
     * uses no-command resource invalidation instead of calling this method.
     */
    void suspend() noexcept;

    /**
     * @brief Create a new rendering context for the retained window.
     *
     * A successful restoration advances `generation()`. Applications then
     * receive `surface_created()` and restore their GPU objects.
     *
     * @return `true` when a usable context was activated.
     */
    [[nodiscard]] bool resume() noexcept;

    /** @brief Refresh drawable dimensions and the backend viewport. */
    void refresh_viewport() noexcept;

    /**
     * @brief Clear the active color buffer.
     * @param color Fill color for the clear.
     */
    void clear(Color color) noexcept;

    /** @brief Present the completed frame. */
    void present() noexcept;

    /**
     * @brief Return whether a usable context exists.
     * @return `true` after `create()` or a successful `resume()`.
     */
    [[nodiscard]] bool valid() const noexcept;

    /**
     * @brief Return the current framebuffer width in pixels.
     * @return Physical pixel width of the active drawable.
     */
    [[nodiscard]] int pixel_width() const noexcept;

    /**
     * @brief Return the current framebuffer height in pixels.
     * @return Physical pixel height of the active drawable.
     */
    [[nodiscard]] int pixel_height() const noexcept;

    /**
     * @brief Monotonic identity of the current native rendering context.
     * @return Generation counter advanced by every successful activation.
     */
    [[nodiscard]] std::uint64_t generation() const noexcept;

    /**
     * @brief Return whether the previous context's GPU objects survived the
     * most recent successful `resume()`.
     * @return `true` when native resources were reported as preserved.
     */
    [[nodiscard]] bool resources_preserved() const noexcept;

private:
    void* window_{nullptr};
    void* native_context_{nullptr};
    int pixel_width_{0};
    int pixel_height_{0};
    std::uint64_t generation_{0};
    bool resources_preserved_{false};
};

}  // namespace squared::graphics
