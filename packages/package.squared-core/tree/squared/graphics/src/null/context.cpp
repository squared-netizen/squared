#include <squared/graphics/context.hpp>

#include <squared/graphics/color.hpp>
#include <squared/graphics/context_config.hpp>

#include <cstdint>

namespace sq::graphics {

/*
 * Headless backend.
 *
 * Every Context symbol resolves and nothing is drawn. It exists so that a
 * project with no rendering backend selected fails as a black screen rather
 * than as a link error naming a private member function, and so that
 * graphics2d, gui and scene2d can be built and tested on a machine with no
 * GPU and no EGL.
 *
 * It answers the same contract as a real backend: create succeeds, valid()
 * becomes true, generation() advances on every activation, and present()
 * returns true. Nothing here allocates.
 */

Context::~Context()
{
    destroy();
}

bool Context::create(const ContextConfig& config) noexcept
{
    window_ = config.native_window;
    native_context_ = nullptr;
    pixel_width_ = config.logical_width;
    pixel_height_ = config.logical_height;
    resources_preserved_ = false;
    ++generation_;
    return true;
}

void Context::destroy() noexcept
{
    window_ = nullptr;
    native_context_ = nullptr;
    pixel_width_ = 0;
    pixel_height_ = 0;
    resources_preserved_ = false;
}

void Context::suspend() noexcept
{
    pixel_width_ = 0;
    pixel_height_ = 0;
    resources_preserved_ = false;
}

bool Context::resume() noexcept
{
    // A headless surface never really goes away, so resources always survive.
    // A real backend decides this from what the platform reports.
    resources_preserved_ = true;
    ++generation_;
    return true;
}

void Context::set_native_window(void* native_window) noexcept
{
    window_ = native_window;
}

void Context::refresh_viewport() noexcept
{
}

void Context::clear(Color color) noexcept
{
    static_cast<void>(color);
}

bool Context::present() noexcept
{
    return valid();
}

bool Context::valid() const noexcept
{
    return generation_ > 0;
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
