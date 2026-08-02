#include <squared/graphics/context.hpp>

#include <SDL.h>
#include <SDL_opengles2_khrplatform.h>
#include <SDL_opengles2_gl2platform.h>
#include <SDL_opengles2_gl2.h>

namespace squared::graphics {

Context::~Context()
{
    destroy();
}

bool Context::create(
    const char* title,
    int logical_width,
    int logical_height
) noexcept
{
    destroy();

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

    window_ = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        logical_width,
        logical_height,
        SDL_WINDOW_OPENGL |
            SDL_WINDOW_SHOWN |
            SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window_) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    native_context_ = SDL_GL_CreateContext(window_);
    if (!native_context_) {
        SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        destroy();
        return false;
    }
    if (SDL_GL_MakeCurrent(window_, native_context_) != 0) {
        SDL_Log("SDL_GL_MakeCurrent failed: %s", SDL_GetError());
        destroy();
        return false;
    }

    if (SDL_GL_SetSwapInterval(1) != 0) {
        SDL_Log(
            "Vertical synchronization is unavailable: %s",
            SDL_GetError()
        );
    }

    refresh_viewport();
    const auto* version = glGetString(GL_VERSION);
    SDL_Log(
        "Squared graphics context: %s",
        version ? reinterpret_cast<const char*>(version) : "unknown"
    );
    if (pixel_width_ <= 0 || pixel_height_ <= 0) {
        SDL_Log("OpenGL ES context has no drawable surface");
        destroy();
        return false;
    }
    return true;
}

void Context::destroy() noexcept
{
    if (native_context_) {
        SDL_GL_DeleteContext(native_context_);
        native_context_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    pixel_width_ = 0;
    pixel_height_ = 0;
}

void Context::refresh_viewport() noexcept
{
    if (!window_ || !native_context_) return;
    SDL_GL_GetDrawableSize(window_, &pixel_width_, &pixel_height_);
    glViewport(0, 0, pixel_width_, pixel_height_);
}

void Context::clear(Color color) noexcept
{
    const Color safe = color.clamped();
    glClearColor(safe.red, safe.green, safe.blue, safe.alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Context::present() noexcept
{
    if (window_) SDL_GL_SwapWindow(window_);
}

bool Context::valid() const noexcept
{
    return window_ && native_context_;
}

int Context::pixel_width() const noexcept
{
    return pixel_width_;
}

int Context::pixel_height() const noexcept
{
    return pixel_height_;
}

}  // namespace squared::graphics
