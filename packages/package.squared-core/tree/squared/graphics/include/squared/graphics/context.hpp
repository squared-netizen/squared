#pragma once

#include <squared/graphics/color.hpp>
#include <squared/graphics/context_config.hpp>

#include <cstdint>

namespace sq::graphics {

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
     * @brief Adopt or create a window and make its rendering context current.
     *
     * @param config Native window handle, or the size and title for a backend
     * that creates its own.
     * @return `true` when the window and context are ready.
     */
    [[nodiscard]] bool create(const ContextConfig& config) noexcept;

    /** @brief Destroy the context and window. */
    void destroy() noexcept;

    /**
     * @brief Release the native rendering context while retaining the window.
     *
     * Callers may release GPU objects before this call while the context is
     * current. On Android this is driven by the platform layer losing the
     * surface, not by application code.
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

    /**
     * @brief Adopt a new native window before the next resume().
     * @param native_window Handle the platform layer received, or null.
     * @note Android destroys the window on backgrounding and hands back a
     * different one, so suspend() and resume() are not enough on their own:
     * the platform layer calls this between them. A backend with no such
     * concept ignores it.
     */
    void set_native_window(void* native_window) noexcept;

    /** @brief Refresh drawable dimensions and the backend viewport. */
    void refresh_viewport() noexcept;

    /**
     * @brief Clear the active color buffer.
     * @param color Fill color for the clear.
     */
    void clear(Color color) noexcept;

    /**
     * @brief Present the completed frame.
     * @return `false` when the surface was lost and the frame did not reach
     * the display. Stop drawing and wait for the platform layer to resume;
     * `generation()` tells you whether GPU objects have to be rebuilt.
     * @note The return value is the only notice some devices give that the
     * surface has gone, which is why it is [[nodiscard]]. Discarding it shows
     * up as an application frozen on its last frame after a phone call.
     */
    [[nodiscard]] bool present() noexcept;

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

} // namespace sq::graphics
