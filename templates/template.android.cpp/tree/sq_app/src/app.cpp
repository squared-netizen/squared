// {{project_name}} — application.
//
// This file is YOURS (ownership class: seeded). Replace the body of it freely.
//
// What it does today: clears to a colour that changes when you touch the
// screen, and counts frames. That is deliberately the smallest thing that
// proves the whole stack is alive — the window came up, EGL bound, events
// arrive, update and render are both being called, and present() is
// succeeding. Delete it as soon as it has told you that.

#include "app.hpp"

#include <squared/app/event.hpp>
#include <squared/app/runtime.hpp>
#include <squared/files/file_handle.hpp>
#include <squared/files/file_system.hpp>
#include <squared/graphics/color.hpp>
#include <squared/graphics/context.hpp>

#include <chrono>
#include <cstdint>

namespace {{project_name}} {

struct App::State {
    sq::graphics::Color background{
        .red = 0.1F, .green = 0.12F, .blue = 0.16F, .alpha = 1.0F
    };

    std::chrono::nanoseconds elapsed{0};
    std::uint64_t frames{0};
    int width{0};
    int height{0};
    bool quit{false};
    bool skin_found{false};
};

App::App()
    : state_(std::make_unique<State>())
{
}

App::~App() = default;

bool App::create(sq::app::Runtime& runtime)
{
    state_->width = runtime.graphics.pixel_width();
    state_->height = runtime.graphics.pixel_height();

    // The default skin ships in the APK. Reading it here proves the whole
    // chain - packaging, the asset index, AndroidAssetFileSystem - before
    // anything is built on top of it. Missing is not fatal: the app still
    // runs, it just has no skin yet.
    state_->skin_found =
        runtime.files.internal("skins/default/skin/uiskin.atlas").exists();

    // Visible without logcat: a dark green start means the bundled skin was
    // found, dark red means it was not. Touching still changes the colour.
    state_->background = state_->skin_found
        ? sq::graphics::Color{.red = 0.08F, .green = 0.22F, .blue = 0.12F,
                              .alpha = 1.0F}
        : sq::graphics::Color{.red = 0.30F, .green = 0.06F, .blue = 0.06F,
                              .alpha = 1.0F};
    return true;
}

void App::handle_event(const sq::app::Event& event)
{
    using Type = sq::app::Event::Type;

    switch (event.type) {
    case Type::PointerDown:
        // Position is in pixels, so dividing by the drawable size gives a
        // zero-to-one value whatever the device.
        if (state_->width > 0 && state_->height > 0) {
            state_->background.red =
                event.x / static_cast<float>(state_->width);
            state_->background.green =
                event.y / static_cast<float>(state_->height);
        }
        break;

    case Type::BackRequested:
    case Type::QuitRequested:
        state_->quit = true;
        break;

    default:
        break;
    }
}

void App::update(std::chrono::nanoseconds delta)
{
    state_->elapsed += delta;
}

void App::render(sq::graphics::Context& graphics)
{
    graphics.clear(state_->background);
    ++state_->frames;
}

void App::pause()
{
}

void App::resume()
{
}

void App::resize(int width, int height)
{
    state_->width = width;
    state_->height = height;
}

void App::surface_created(sq::graphics::Context& graphics)
{
    state_->width = graphics.pixel_width();
    state_->height = graphics.pixel_height();

    // When resources_preserved() is false, every texture, buffer and shader
    // this application owned is gone and has to be rebuilt. There is nothing
    // to rebuild yet; there will be.
    if (!graphics.resources_preserved()) {
        // rebuild GPU objects here
    }
}

void App::surface_destroyed()
{
}

void App::dispose()
{
}

bool App::quit_requested() const noexcept
{
    return state_->quit;
}

}  // namespace {{project_name}}
