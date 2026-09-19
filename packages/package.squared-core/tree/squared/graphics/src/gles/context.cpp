#include <squared/graphics/context.hpp>

#include <squared/graphics/color.hpp>
#include <squared/graphics/context_config.hpp>

#include <cstdint>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#if defined(__ANDROID__)
/*
 * One function is needed from <android/native_window.h>, and that header pulls
 * in android/hardware_buffer.h, android/data_space.h and android/rect.h for
 * declarations nothing here uses. ANativeWindow is already forward declared by
 * EGL/eglplatform.h on Android, so the declaration below is all that is
 * missing.
 *
 * ANativeWindow_setBuffersGeometry has been public and stable since API 1. If
 * a future NDK ever changes it, this is a compile error at exactly this line,
 * not a silent mismatch: replace it with the include and add the three headers
 * above to the build.
 */
extern "C" std::int32_t ANativeWindow_setBuffersGeometry(
    ANativeWindow* window,
    std::int32_t width,
    std::int32_t height,
    std::int32_t format
);
#endif

namespace sq::graphics {

/*
 * EGL + GLES 3.0 backend.
 *
 * The Context members are reused rather than extended, because the class has a
 * fixed layout declared in the public header: window_ holds the
 * ANativeWindow*, native_context_ holds the EGLContext. The display, config
 * and surface are file-scope state, which is correct here for a reason worth
 * stating: EGL_DEFAULT_DISPLAY is one display per process, and Android gives a
 * process one native window at a time. A second Context would be a second
 * window, which NativeActivity does not offer.
 */

namespace {

EGLDisplay g_display = EGL_NO_DISPLAY;
EGLSurface g_surface = EGL_NO_SURFACE;
EGLConfig g_config = nullptr;

/*
 * RGBA8 with a 16-bit depth buffer and 8-bit stencil.
 *
 * Depth and stencil are requested even though the 2D path does not use them:
 * a config without them cannot be added later without recreating the surface,
 * and every GLES 3.0 device offers this combination. Asking for more than is
 * needed costs bandwidth; asking for less costs a surface rebuild the first
 * time anything wants a stencil clip.
 */
constexpr EGLint k_config_attributes[] = {
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 16,
    EGL_STENCIL_SIZE, 8,
    EGL_NONE
};

constexpr EGLint k_context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 3,
    EGL_NONE
};

/** @brief Bring up the display and choose a config, once per process. */
bool ensure_display() noexcept
{
    if (g_display != EGL_NO_DISPLAY) return true;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) return false;
    if (eglInitialize(display, nullptr, nullptr) != EGL_TRUE) return false;

    EGLint config_count = 0;
    EGLConfig config = nullptr;
    if (eglChooseConfig(
            display, k_config_attributes, &config, 1, &config_count
        ) != EGL_TRUE
        || config_count < 1) {
        eglTerminate(display);
        return false;
    }

    g_display = display;
    g_config = config;
    return true;
}

/**
 * @brief Create the window surface for a native window.
 *
 * The buffer geometry is set from the config's native visual id first. Skipping
 * that step is the classic Android EGL bug: the surface is created against a
 * buffer format the config did not ask for, and the result is either a failed
 * eglMakeCurrent or colours that come out wrong on some devices only.
 */
bool ensure_surface(void* native_window) noexcept
{
    if (g_surface != EGL_NO_SURFACE) return true;
    if (native_window == nullptr) return false;

    auto* window = static_cast<ANativeWindow*>(native_window);

#if defined(__ANDROID__)
    EGLint visual_id = 0;
    if (eglGetConfigAttrib(
            g_display, g_config, EGL_NATIVE_VISUAL_ID, &visual_id
        ) == EGL_TRUE) {
        static_cast<void>(ANativeWindow_setBuffersGeometry(
            window, 0, 0, static_cast<std::int32_t>(visual_id)
        ));
    }
#endif

    g_surface = eglCreateWindowSurface(g_display, g_config, window, nullptr);
    return g_surface != EGL_NO_SURFACE;
}

void destroy_surface() noexcept
{
    if (g_display != EGL_NO_DISPLAY && g_surface != EGL_NO_SURFACE) {
        eglDestroySurface(g_display, g_surface);
    }
    g_surface = EGL_NO_SURFACE;
}

}  // namespace

Context::~Context()
{
    destroy();
}

bool Context::create(const ContextConfig& config) noexcept
{
    if (!ensure_display()) return false;
    if (!ensure_surface(config.native_window)) return false;

    auto context = eglCreateContext(
        g_display, g_config, EGL_NO_CONTEXT, k_context_attributes
    );
    if (context == EGL_NO_CONTEXT) {
        destroy_surface();
        return false;
    }

    if (eglMakeCurrent(g_display, g_surface, g_surface, context) != EGL_TRUE) {
        eglDestroyContext(g_display, context);
        destroy_surface();
        return false;
    }

    window_ = config.native_window;
    native_context_ = context;
    resources_preserved_ = false;
    ++generation_;
    refresh_viewport();
    return true;
}

void Context::destroy() noexcept
{
    if (g_display != EGL_NO_DISPLAY) {
        static_cast<void>(eglMakeCurrent(
            g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT
        ));
        if (native_context_ != nullptr) {
            eglDestroyContext(
                g_display, static_cast<EGLContext>(native_context_)
            );
        }
        destroy_surface();
    }

    window_ = nullptr;
    native_context_ = nullptr;
    pixel_width_ = 0;
    pixel_height_ = 0;
    resources_preserved_ = false;
}

void Context::suspend() noexcept
{
    // The context is kept; only the surface goes. Android hands back a new
    // ANativeWindow on resume, and a context that was never destroyed usually
    // keeps its objects, which is what resources_preserved() reports.
    if (g_display != EGL_NO_DISPLAY) {
        static_cast<void>(eglMakeCurrent(
            g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT
        ));
        destroy_surface();
    }
    pixel_width_ = 0;
    pixel_height_ = 0;
    resources_preserved_ = false;
}

bool Context::resume() noexcept
{
    if (!ensure_display()) return false;
    if (!ensure_surface(window_)) return false;

    // Try the surviving context first. When it still binds, every GPU object
    // it held is still there and nothing has to be rebuilt.
    if (native_context_ != nullptr) {
        auto existing = static_cast<EGLContext>(native_context_);
        if (eglMakeCurrent(g_display, g_surface, g_surface, existing)
            == EGL_TRUE) {
            resources_preserved_ = true;
            ++generation_;
            refresh_viewport();
            return true;
        }
        eglDestroyContext(g_display, existing);
        native_context_ = nullptr;
    }

    auto context = eglCreateContext(
        g_display, g_config, EGL_NO_CONTEXT, k_context_attributes
    );
    if (context == EGL_NO_CONTEXT) {
        destroy_surface();
        return false;
    }
    if (eglMakeCurrent(g_display, g_surface, g_surface, context) != EGL_TRUE) {
        eglDestroyContext(g_display, context);
        destroy_surface();
        return false;
    }

    native_context_ = context;
    resources_preserved_ = false;
    ++generation_;
    refresh_viewport();
    return true;
}

void Context::set_native_window(void* native_window) noexcept
{
    if (native_window == window_) return;
    destroy_surface();
    window_ = native_window;
}

void Context::refresh_viewport() noexcept
{
    if (g_display == EGL_NO_DISPLAY || g_surface == EGL_NO_SURFACE) return;

    EGLint width = 0;
    EGLint height = 0;
    static_cast<void>(eglQuerySurface(g_display, g_surface, EGL_WIDTH, &width));
    static_cast<void>(
        eglQuerySurface(g_display, g_surface, EGL_HEIGHT, &height)
    );

    pixel_width_ = static_cast<int>(width);
    pixel_height_ = static_cast<int>(height);
    glViewport(0, 0, width, height);
}

void Context::clear(Color color) noexcept
{
    glClearColor(color.red, color.green, color.blue, color.alpha);
    glClear(
        static_cast<GLbitfield>(GL_COLOR_BUFFER_BIT)
        | static_cast<GLbitfield>(GL_DEPTH_BUFFER_BIT)
        | static_cast<GLbitfield>(GL_STENCIL_BUFFER_BIT)
    );
}

bool Context::present() noexcept
{
    if (g_display == EGL_NO_DISPLAY || g_surface == EGL_NO_SURFACE) {
        return false;
    }
    if (eglSwapBuffers(g_display, g_surface) == EGL_TRUE) return true;

    // The swap failed. EGL_BAD_SURFACE and EGL_CONTEXT_LOST both mean stop
    // drawing: the platform layer will suspend and resume, and generation()
    // will tell the application whether its GPU objects survived.
    const EGLint error = eglGetError();
    if (error == EGL_BAD_SURFACE || error == EGL_CONTEXT_LOST) {
        destroy_surface();
    }
    return false;
}

bool Context::valid() const noexcept
{
    return native_context_ != nullptr && g_surface != EGL_NO_SURFACE;
}

int Context::pixel_width() const noexcept
{
    return pixel_width_;
}

int Context::pixel_height() const noexcept
{
    return pixel_height_;
}

std::uint64_t Context::generation() const noexcept
{
    return generation_;
}

bool Context::resources_preserved() const noexcept
{
    return resources_preserved_;
}

}  // namespace sq::graphics
