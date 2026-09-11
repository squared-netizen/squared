// {{project_name}} — Android platform layer, on SFML.
//
// This file is YOURS (ownership class: seeded). The generator wrote it once and
// will not overwrite it. It is the expert escape hatch: everything about how
// this project meets Android is here, and you may change all of it.
//
// You should not normally need to. Application code goes in sq_app/, which
// includes no Android header and no SFML header, and compiles unchanged under
// template.android.cpp with its NativeActivity platform layer.
//
// ## Who owns the entry point
//
// Under template.android.cpp this file defines android_main() and drives
// android_native_app_glue. Here it does not, because SFML will not share that
// role: sfml-main defines ANativeActivity_onCreate itself, installs its own
// activity callbacks, and then starts your main() on a thread it owns. There is
// one ANativeActivity_onCreate slot per process, so the glue and SFML cannot
// both be present.
//
// That is why this is a separate template rather than a kit applied to the
// other one. A kit adds to a workspace; it does not replace the platform layer.
//
// ## The thread you are on
//
// main() below does NOT run on Android's UI thread. SFML's activity glue
// receives the lifecycle and window callbacks on the UI thread, records them,
// and runs this function on a detached thread of its own. sf::Window reads that
// recorded state.
//
// Practical consequences:
//
//   * Blocking here will not trigger an ANR the way blocking the UI thread
//     would. Do not take that as licence: a blocked loop still stops drawing
//     and stops draining events.
//   * Every App method below is called from this one thread, so App itself
//     needs no synchronisation. That is the same guarantee the NativeActivity
//     template gives, reached a different way.
//   * JNI from here needs an attached environment. SFML attaches its own; if
//     you add JNI calls of your own, attach and detach them yourself.

#include "app.hpp"

#include <SFML/Window.hpp>

#include <android/log.h>

#include <optional>

namespace {

constexpr const char* kTag = "{{project_name}}";

#define SQ_LOGI(...) __android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__)
#define SQ_LOGW(...) __android_log_print(ANDROID_LOG_WARN, kTag, __VA_ARGS__)

/// Translate one SFML event into calls on the application.
///
/// Returns false when the application should stop. Kept separate from the loop
/// so the mapping is one readable table rather than a switch buried in a while.
bool dispatch(const sf::Event& event, {{project_name}}::App& app, bool& focused)
{
    using namespace {{project_name}};

    if (event.is<sf::Event::Closed>())
        return false;

    if (const auto* resized = event.getIf<sf::Event::Resized>())
    {
        app.resize(static_cast<int>(resized->size.x), static_cast<int>(resized->size.y));
        return true;
    }

    if (event.is<sf::Event::FocusGained>())
    {
        // Android delivers focus changes on every return from the background.
        // Guarded because SFML can report a gain without an intervening loss.
        if (!focused)
        {
            focused = true;
            app.resume();
        }
        return true;
    }

    if (event.is<sf::Event::FocusLost>())
    {
        if (focused)
        {
            focused = false;
            app.pause();
        }
        return true;
    }

    // Only the first finger is forwarded. App::touch carries no finger index,
    // deliberately: it is the portable contract, and multi-touch differs enough
    // between platforms that it belongs here rather than in sq_app/. If you
    // need it, widen the contract in app.hpp and handle it in both templates.
    if (const auto* began = event.getIf<sf::Event::TouchBegan>())
    {
        if (began->finger == 0)
            app.touch(TouchPhase::began,
                      static_cast<float>(began->position.x),
                      static_cast<float>(began->position.y));
        return true;
    }

    if (const auto* moved = event.getIf<sf::Event::TouchMoved>())
    {
        if (moved->finger == 0)
            app.touch(TouchPhase::moved,
                      static_cast<float>(moved->position.x),
                      static_cast<float>(moved->position.y));
        return true;
    }

    if (const auto* ended = event.getIf<sf::Event::TouchEnded>())
    {
        if (ended->finger == 0)
            app.touch(TouchPhase::ended,
                      static_cast<float>(ended->position.x),
                      static_cast<float>(ended->position.y));
        return true;
    }

    return true;
}

}  // namespace

/// The entry point SFML calls.
///
/// Named main() because sfml-main's ANativeActivity_onCreate looks it up as
/// `extern int main(int, char**)` and starts it on its own thread. The symbol
/// must exist with that signature or the link fails inside SFML rather than
/// here.
int main()
{
    SQ_LOGI("starting");

    // Fullscreen at the device's own resolution. On Android SFML ignores the
    // mode and takes the whole surface regardless; passing the desktop mode
    // keeps this line meaningful if the same platform layer is ever pointed at
    // a desktop build.
    sf::Window window(sf::VideoMode::getDesktopMode(), kTag, sf::State::Fullscreen);

    window.setVerticalSyncEnabled(true);

    {{project_name}}::App app;
    app.start();

    // Before the first frame, and matching what App::resize promises: called
    // once up front, then again on every change.
    {
        const sf::Vector2u size = window.getSize();
        app.resize(static_cast<int>(size.x), static_cast<int>(size.y));
        SQ_LOGI("surface %ux%u", size.x, size.y);
    }

    bool focused = true;
    bool running = true;

    while (running && window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (!dispatch(*event, app, focused))
            {
                running = false;
                break;
            }
        }

        // Backgrounded: no surface to draw into, and drawing anyway would burn
        // battery rendering into nothing. waitEvent blocks until Android has
        // something to say, which is the whole cost of being in the background.
        if (!focused)
        {
            if (const std::optional event = window.waitEvent())
            {
                if (!dispatch(*event, app, focused))
                    running = false;
            }
            continue;
        }

        app.render();
        window.display();
    }

    SQ_LOGI("shutting down");
    app.stop();
    window.close();
    return 0;
}
