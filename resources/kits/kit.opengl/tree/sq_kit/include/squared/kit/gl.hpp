// SPDX-License-Identifier: MIT
//
// squared/kit/gl.hpp — OpenGL ES 3.0 bridge for the Squared framework.
//
// Contributed by kit.opengl. A *bridge*: it connects a Squared application to
// EGL and GLES on Android, the way kit.terminal connects one to the console.
//
// GENERATED FILE. The generator owns this path and will replace it on update.
// Write your own code in the working directory instead.
//
// Scope, phase 1: context management and clearing. Bringing up EGL correctly
// is the part that is fiddly and easy to get subtly wrong — config selection,
// surface recreation on rotation, context loss when the app is backgrounded.
// Shaders, buffers and textures follow once this is proven on-device.
//
// Deliberately absent, and staying absent: image decoding (an asset concern —
// a decoder would make a "basics" kit non-trivial to audit), math types (a
// framework concern), and ownership of the frame loop (sq_android/entry.cpp
// owns it, so you can replace it).
//
// GLES 3.0 is the floor: API 18+, effectively every live device. Shaders are
// GLSL ES 3.00 (`#version 300 es`), which requires explicit precision
// qualifiers in fragment shaders — the first thing that catches people coming
// from desktop GLSL.

#ifndef SQUARED_KIT_GL_HPP
#define SQUARED_KIT_GL_HPP

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <android/log.h>
#include <android/native_window.h>

#include <string>
#include <utility>

namespace sq::gl {

/// Whether this build has GLES available. Always true when the header is
/// reachable; provided so application code can branch on one name rather than
/// on `__has_include`.
inline constexpr bool available = true;

/// An RGBA colour, components in [0, 1].
struct Color {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};
};

/// The outcome of an operation that can fail for reasons outside your control
/// — a lost context, a surface that vanished, a driver that refused a config.
///
/// A value rather than an exception, for the same reason the engine makes that
/// choice: on Android these are ordinary events, not exceptional ones. The app
/// is backgrounded, the context goes away, and the correct response is to
/// carry on and rebuild it, not to unwind.
struct Result {
    bool        ok{false};
    std::string message;

    explicit operator bool() const noexcept { return ok; }

    static Result success() { return Result{true, {}}; }
    static Result failure(std::string why) { return Result{false, std::move(why)}; }
};

/// Human-readable name for an EGL error code.
[[nodiscard]] inline const char* egl_error_string(EGLint error) noexcept {
    switch (error) {
        case EGL_SUCCESS: return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
        case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
        case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
        case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
        case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
        case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
        case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
        case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
        default: return "unknown EGL error";
    }
}

/// Human-readable name for a GL error code.
[[nodiscard]] inline const char* error_string(GLenum error) noexcept {
    switch (error) {
        case GL_NO_ERROR: return "GL_NO_ERROR";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        default: return "unknown GL error";
    }
}

/// An EGL display, config, surface and context, bound to one ANativeWindow.
///
/// Move-only and RAII. Android hands a window over and takes it away again —
/// on rotation, on backgrounding — so create() and destroy() may each be
/// called many times over one process lifetime, and both are safe to call
/// when there is nothing to do.
class Context {
public:
    Context() = default;
    ~Context() { destroy(); }

    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;

    Context(Context&& other) noexcept { steal(other); }

    Context& operator=(Context&& other) noexcept {
        if (this != &other) {
            destroy();
            steal(other);
        }
        return *this;
    }

    /// Bring up a rendering surface on `window`.
    ///
    /// Safe to call when a surface already exists: the old one is torn down
    /// first. That matters because APP_CMD_INIT_WINDOW can arrive more than
    /// once without an intervening TERM.
    [[nodiscard]] Result create(ANativeWindow* window) {
        if (window == nullptr) return Result::failure("no native window");
        destroy();

        display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display_ == EGL_NO_DISPLAY) return fail("eglGetDisplay");

        EGLint major = 0;
        EGLint minor = 0;
        if (eglInitialize(display_, &major, &minor) == EGL_FALSE) return fail("eglInitialize");

        // 8/8/8 colour, 24-bit depth, 8-bit stencil, GLES 3. No multisampling:
        // it is a per-application decision with a real cost on mobile, and a
        // kit that enabled it by default would be making that call for you.
        const EGLint config_attributes[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
            EGL_RED_SIZE,        8,
            EGL_GREEN_SIZE,      8,
            EGL_BLUE_SIZE,       8,
            EGL_ALPHA_SIZE,      8,
            EGL_DEPTH_SIZE,      24,
            EGL_STENCIL_SIZE,    8,
            EGL_NONE};

        EGLint config_count = 0;
        if (eglChooseConfig(display_, config_attributes, &config_, 1, &config_count) == EGL_FALSE ||
            config_count < 1) {
            // Fall back to no depth or stencil before giving up. Some older
            // drivers refuse the full request and accept the reduced one, and
            // a blank screen is a poor way to learn that.
            const EGLint minimal[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
                EGL_RED_SIZE,        8,
                EGL_GREEN_SIZE,      8,
                EGL_BLUE_SIZE,       8,
                EGL_NONE};
            if (eglChooseConfig(display_, minimal, &config_, 1, &config_count) == EGL_FALSE ||
                config_count < 1) {
                return fail("eglChooseConfig: no GLES 3 config available");
            }
        }

        // ANativeWindow must be reconfigured to the format EGL picked, or the
        // surface is created and renders nothing visible.
        EGLint format = 0;
        eglGetConfigAttrib(display_, config_, EGL_NATIVE_VISUAL_ID, &format);
        ANativeWindow_setBuffersGeometry(window, 0, 0, format);

        surface_ = eglCreateWindowSurface(display_, config_, window, nullptr);
        if (surface_ == EGL_NO_SURFACE) return fail("eglCreateWindowSurface");

        const EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attributes);
        if (context_ == EGL_NO_CONTEXT) return fail("eglCreateContext");

        if (eglMakeCurrent(display_, surface_, surface_, context_) == EGL_FALSE) {
            return fail("eglMakeCurrent");
        }

        eglQuerySurface(display_, surface_, EGL_WIDTH, &width_);
        eglQuerySurface(display_, surface_, EGL_HEIGHT, &height_);
        glViewport(0, 0, width_, height_);

        window_ = window;
        return Result::success();
    }

    /// Tear the surface down. Safe when there is nothing to tear down.
    void destroy() noexcept {
        if (display_ == EGL_NO_DISPLAY) return;

        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) eglDestroyContext(display_, context_);
        if (surface_ != EGL_NO_SURFACE) eglDestroySurface(display_, surface_);
        eglTerminate(display_);

        display_ = EGL_NO_DISPLAY;
        surface_ = EGL_NO_SURFACE;
        context_ = EGL_NO_CONTEXT;
        window_  = nullptr;
        width_   = 0;
        height_  = 0;
    }

    [[nodiscard]] bool valid() const noexcept { return context_ != EGL_NO_CONTEXT; }
    [[nodiscard]] int  width() const noexcept { return width_; }
    [[nodiscard]] int  height() const noexcept { return height_; }

    /// Make this context current on the calling thread. Cheap when it already
    /// is; call it every frame rather than tracking currency yourself.
    [[nodiscard]] bool make_current() noexcept {
        if (!valid()) return false;
        return eglMakeCurrent(display_, surface_, surface_, context_) == EGL_TRUE;
    }

    /// Swap buffers.
    ///
    /// Returns false when the surface has been lost, which is normal rather
    /// than exceptional: it happens whenever the app is backgrounded. The
    /// caller should stop drawing and wait for a new window.
    [[nodiscard]] bool present() noexcept {
        if (!valid()) return false;
        if (eglSwapBuffers(display_, surface_) == EGL_TRUE) return true;

        const EGLint error = eglGetError();
        if (error == EGL_BAD_SURFACE || error == EGL_CONTEXT_LOST) {
            __android_log_print(ANDROID_LOG_INFO, "sq.gl", "surface lost (%s)",
                                egl_error_string(error));
            destroy();
        }
        return false;
    }

    /// Renderer, vendor and GLES version, for the log.
    [[nodiscard]] std::string description() const {
        if (!valid()) return "no context";
        const auto text = [](GLenum name) -> const char* {
            const auto* value = glGetString(name);
            return value != nullptr ? reinterpret_cast<const char*>(value) : "?";
        };
        return std::string{text(GL_RENDERER)} + " / " + text(GL_VERSION);
    }

private:
    void steal(Context& other) noexcept {
        display_ = std::exchange(other.display_, EGL_NO_DISPLAY);
        surface_ = std::exchange(other.surface_, EGL_NO_SURFACE);
        context_ = std::exchange(other.context_, EGL_NO_CONTEXT);
        config_  = other.config_;
        window_  = std::exchange(other.window_, nullptr);
        width_   = std::exchange(other.width_, 0);
        height_  = std::exchange(other.height_, 0);
    }

    /// Build a failure carrying the EGL error, then tear down. Half a context
    /// is worse than none: leaving one behind would make the next create()
    /// fail for a reason unrelated to what actually went wrong.
    [[nodiscard]] Result fail(const char* stage) {
        const EGLint error = eglGetError();
        std::string  message = std::string{stage} + ": " + egl_error_string(error);
        destroy();
        return Result::failure(std::move(message));
    }

    EGLDisplay     display_{EGL_NO_DISPLAY};
    EGLSurface     surface_{EGL_NO_SURFACE};
    EGLContext     context_{EGL_NO_CONTEXT};
    EGLConfig      config_{nullptr};
    ANativeWindow* window_{nullptr};
    int            width_{0};
    int            height_{0};
};

// ---------------------------------------------------------------------------
// Immediate operations
//
// Free functions rather than Context members: they act on whatever context is
// current, which is what the underlying calls do. Pretending otherwise would
// suggest a per-context state this kit does not track.
// ---------------------------------------------------------------------------

inline void clear(Color colour) {
    glClearColor(colour.r, colour.g, colour.b, colour.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void viewport(int width, int height) {
    glViewport(0, 0, width, height);
}

/// Drain and report the GL error queue. Returns true if anything was wrong.
///
/// GL errors are sticky and accumulate silently; something has to ask. Call
/// this after a block of setup while developing, and delete the call when the
/// block is known good.
inline bool check_errors(const char* where) {
    bool   any = false;
    GLenum error = glGetError();
    while (error != GL_NO_ERROR) {
        __android_log_print(ANDROID_LOG_ERROR, "sq.gl", "%s: %s", where, error_string(error));
        any   = true;
        error = glGetError();
    }
    return any;
}

}  // namespace sq::gl

#endif  // SQUARED_KIT_GL_HPP
