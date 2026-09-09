// {{project_name}} — Android platform entry point.
//
// This file is YOURS (ownership class: seeded). The generator wrote it once and
// will not overwrite it. It is the expert escape hatch: everything about how
// this project meets Android is here, and you may change all of it.
//
// You should not normally need to. Application code goes in sq_app/, which
// includes no Android header and compiles unchanged under other templates.
//
// What this file does:
//
//   * runs the android_native_app_glue event loop
//   * translates lifecycle and input events into calls on your App
//   * brings up a rendering surface, IF a rendering kit was applied
//
// The renderer is optional by design. The include below is guarded, so this
// project builds and runs with no rendering kit at all — you get a black
// window and a log line. That is what lets kit.opengl and a future kit.sfml be
// alternatives rather than one of them being baked in.

#include "app.hpp"

#include <android/log.h>
#include <android_native_app_glue.h>

#if __has_include(<opengl/gl.hpp>)
#  include <opengl/gl.hpp>
#  define SQ_HAS_RENDERER 1
#endif

#include <cstdint>
#include <memory>

namespace {

constexpr const char* kTag = "{{project_name}}";

#define SQ_LOGI(...) __android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__)
#define SQ_LOGW(...) __android_log_print(ANDROID_LOG_WARN, kTag, __VA_ARGS__)

/// Everything the platform layer owns for the lifetime of the process.
///
/// Held in one struct rather than as globals so that the glue's userData
/// pointer is the only piece of shared state, and so ownership is obvious when
/// you come to change this file.
struct Platform {
    {{project_name}}::App app;

    bool has_focus{false};
    bool surface_ready{false};

#if defined(SQ_HAS_RENDERER)
    sq::gl::Context renderer;
#endif

    /// Bring the rendering surface up. Called when Android hands us a window,
    /// which can happen more than once in a process — on rotation, or after
    /// the app returns from the background.
    void attach(ANativeWindow* window) {
#if defined(SQ_HAS_RENDERER)
        const sq::gl::Result created = renderer.create(window);
        if (!created) {
            SQ_LOGW("renderer: %s", created.message.c_str());
            surface_ready = false;
            return;
        }
        surface_ready = true;
        SQ_LOGI("renderer: %s, %dx%d", renderer.description().c_str(),
                renderer.width(), renderer.height());
        app.resize(renderer.width(), renderer.height());
#else
        (void)window;
        surface_ready = false;
        SQ_LOGW("no rendering kit was applied; the window stays blank");
        SQ_LOGW("add one at generation time: --kit kit.opengl");
#endif
    }

    void detach() {
#if defined(SQ_HAS_RENDERER)
        renderer.destroy();
#endif
        surface_ready = false;
    }

    /// One frame. Does nothing when there is no surface, which is the normal
    /// state while backgrounded — drawing then would be wasted work at best.
    void frame() {
        if (!surface_ready) return;
#if defined(SQ_HAS_RENDERER)
        if (!renderer.make_current()) return;
        app.render();

        // present() returns false when the surface has been lost, which is
        // normal rather than exceptional -- it happens every time the app is
        // backgrounded. It has already torn the context down, so the only
        // correct response is to stop drawing and wait for a new window.
        // Ignoring the result would spin, rendering into nothing, until
        // Android killed the process.
        if (!renderer.present()) surface_ready = false;
#endif
    }
};

Platform& platform_of(android_app* app) {
    return *static_cast<Platform*>(app->userData);
}

void on_command(android_app* app, int32_t command) {
    Platform& platform = platform_of(app);

    switch (command) {
        case APP_CMD_INIT_WINDOW:
            SQ_LOGI("window created");
            if (app->window != nullptr) platform.attach(app->window);
            break;

        case APP_CMD_TERM_WINDOW:
            SQ_LOGI("window destroyed");
            platform.detach();
            break;

        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONFIG_CHANGED:
            // A configuration change with the same window still needs the
            // surface re-measured: rotation changes the drawable size without
            // destroying the window.
            if (platform.surface_ready && app->window != nullptr) {
                platform.detach();
                platform.attach(app->window);
            }
            break;

        case APP_CMD_GAINED_FOCUS:
            platform.has_focus = true;
            platform.app.resume();
            break;

        case APP_CMD_LOST_FOCUS:
            platform.has_focus = false;
            platform.app.pause();
            break;

        case APP_CMD_SAVE_STATE:
            // Nothing to persist yet. When you add state, allocate it with
            // malloc into app->savedState and set app->savedStateSize; the
            // glue frees it for you.
            break;

        case APP_CMD_DESTROY:
            SQ_LOGI("destroy");
            break;

        default:
            break;
    }
}

int32_t on_input(android_app* app, AInputEvent* event) {
    Platform& platform = platform_of(app);

    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;

    const int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    const float x = AMotionEvent_getX(event, 0);
    const float y = AMotionEvent_getY(event, 0);

    switch (action) {
        case AMOTION_EVENT_ACTION_DOWN:
            return platform.app.touch({{project_name}}::TouchPhase::began, x, y) ? 1 : 0;
        case AMOTION_EVENT_ACTION_MOVE:
            return platform.app.touch({{project_name}}::TouchPhase::moved, x, y) ? 1 : 0;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_CANCEL:
            return platform.app.touch({{project_name}}::TouchPhase::ended, x, y) ? 1 : 0;
        default:
            return 0;
    }
}

}  // namespace

/// The glue's entry point. Named by android_native_app_glue, not by us.
void android_main(android_app* app) {
    SQ_LOGI("starting");

    // Heap-allocated because the glue's userData is a void*, and because
    // Platform holds the renderer, which must be destroyed before this
    // function returns.
    auto platform = std::make_unique<Platform>();
    app->userData     = platform.get();
    app->onAppCmd     = on_command;
    app->onInputEvent = on_input;

    platform->app.start();

    while (true) {
        int                  events = 0;
        android_poll_source* source = nullptr;

        // Block only when there is nothing to draw. With a live surface we
        // poll with a zero timeout and render continuously; without one we
        // wait, so a backgrounded app costs no battery.
        const int timeout = platform->surface_ready ? 0 : -1;

        while (ALooper_pollOnce(timeout, nullptr, &events,
                                reinterpret_cast<void**>(&source)) >= 0) {
            if (source != nullptr) source->process(app, source);

            if (app->destroyRequested != 0) {
                SQ_LOGI("shutting down");
                platform->app.stop();
                platform->detach();
                app->userData = nullptr;
                return;
            }
            if (platform->surface_ready) break;
        }

        platform->frame();
    }
}
