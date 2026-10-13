#pragma once

namespace sq::graphics {

/**
 * @brief What a Context needs to come up, supplied by the platform layer.
 *
 * The native window comes from the platform rather than from a title and a
 * size, because that is the only form that generalises. Android hands the
 * framework an ANativeWindow it did not create and cannot name; a desktop
 * backend creates its own and wants a title. A config carries both, and each
 * backend reads the fields that mean something to it.
 */
struct ContextConfig final {
    /**
     * @brief Native window handle the context should adopt.
     *
     * `ANativeWindow*` on Android, cast to `void*` so no squared header
     * includes a platform header. Null asks the backend to create its own
     * window, which only a desktop backend can honour.
     */
    void* native_window{nullptr};

    /** @brief Window title, where the platform has one. */
    const char* title{"squared"};

    /** @brief Initial logical width, used when the backend creates a window. */
    int logical_width{1280};

    /** @brief Initial logical height, used when the backend creates a window. */
    int logical_height{720};
};

}  // namespace sq::graphics
