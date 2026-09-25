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
//   * owns the sq::graphics::Context and its surface lifetime
//   * owns the file system and asset manager, and hands the application a
//     sq::app::Runtime referring to all three
//   * translates NativeActivity lifecycle and input into sq::app::Event
//   * measures frame time and drives update() separately from render()
//
// There is no longer an "is a renderer present" question. sq::graphics::Context
// always exists; which backend answers it is chosen at link time, and the null
// backend draws nothing rather than failing to link. A project with no GLES
// gets a black window and a log line, which is what the old __has_include
// guard was for.

#include "app.hpp"

#include <squared/app/event.hpp>
#include <squared/app/runtime.hpp>
#include <squared/assets/asset_manager.hpp>
#include <squared/files/android_asset_file_system.hpp>
#include <squared/app/key_modifier.hpp>
#include <squared/graphics/context.hpp>
#include <squared/graphics/context_config.hpp>

#include <android/log.h>
#include <android/keycodes.h>
#include <android_native_app_glue.h>

#include <chrono>
#include <cstdint>
#include <memory>

namespace {

constexpr const char* kTag = "{{project_name}}";

#define SQ_LOGI(...) __android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__)
#define SQ_LOGW(...) __android_log_print(ANDROID_LOG_WARN, kTag, __VA_ARGS__)

using Event = sq::app::Event;
using Clock = std::chrono::steady_clock;

/// Translate an Android keycode into squared's portable key.
///
/// Only the keys squared names. Everything else arrives as Key::unknown with
/// the Android code still in input_device_id, so an application that needs a
/// key squared does not name can still find it.
Event::Key translate_key(std::int32_t code) {
    switch (code) {
        case AKEYCODE_DPAD_LEFT:  return Event::Key::left;
        case AKEYCODE_DPAD_RIGHT: return Event::Key::right;
        case AKEYCODE_DPAD_UP:    return Event::Key::up;
        case AKEYCODE_DPAD_DOWN:  return Event::Key::down;
        case AKEYCODE_MOVE_HOME:  return Event::Key::home;
        case AKEYCODE_MOVE_END:   return Event::Key::end;
        case AKEYCODE_DEL:        return Event::Key::backspace;
        case AKEYCODE_FORWARD_DEL:return Event::Key::delete_key;
        case AKEYCODE_ENTER:
        case AKEYCODE_NUMPAD_ENTER:
        case AKEYCODE_DPAD_CENTER:return Event::Key::enter;
        case AKEYCODE_SPACE:      return Event::Key::space;
        case AKEYCODE_TAB:        return Event::Key::tab;
        case AKEYCODE_ESCAPE:     return Event::Key::escape;
        default:                  return Event::Key::unknown;
    }
}

/// Translate an Android keycode into a semantic navigation action.
///
/// A D-pad or a game controller drives focus movement without the application
/// having to know which key did it. This is what makes a squared GUI navigable
/// on a TV or with a gamepad attached.
Event::Navigation translate_navigation(std::int32_t code) {
    switch (code) {
        case AKEYCODE_DPAD_LEFT:  return Event::Navigation::left;
        case AKEYCODE_DPAD_RIGHT: return Event::Navigation::right;
        case AKEYCODE_DPAD_UP:    return Event::Navigation::up;
        case AKEYCODE_DPAD_DOWN:  return Event::Navigation::down;
        case AKEYCODE_TAB:        return Event::Navigation::next;
        case AKEYCODE_DPAD_CENTER:
        case AKEYCODE_ENTER:      return Event::Navigation::activate;
        case AKEYCODE_BACK:
        case AKEYCODE_ESCAPE:     return Event::Navigation::cancel;
        default:                  return Event::Navigation::unknown;
    }
}

sq::app::KeyModifiers translate_modifiers(std::int32_t state) {
    sq::app::KeyModifiers modifiers;
    modifiers.set(sq::app::KeyModifier::shift, (state & AMETA_SHIFT_ON) != 0);
    modifiers.set(sq::app::KeyModifier::control, (state & AMETA_CTRL_ON) != 0);
    modifiers.set(sq::app::KeyModifier::alt, (state & AMETA_ALT_ON) != 0);
    modifiers.set(sq::app::KeyModifier::meta, (state & AMETA_META_ON) != 0);
    return modifiers;
}

/// Storage roots from the activity.
///
/// Android's "internal storage" - internalDataPath - is squared's Local:
/// private, writable, no permission needed, removed on uninstall. squared's
/// Internal is the read-only asset bundle inside the APK, which the file
/// system reaches through the asset manager instead.
///
/// externalDataPath is null when external storage is not mounted. An empty
/// root makes every External path fail cleanly rather than resolve somewhere
/// unintended.
sq::files::AndroidStorageRoots storage_roots(const android_app* native) {
    const ANativeActivity* activity = native->activity;
    return sq::files::AndroidStorageRoots{
        .local_root = activity->internalDataPath != nullptr
                          ? activity->internalDataPath : "",
        .external_root = activity->externalDataPath != nullptr
                             ? activity->externalDataPath : ""};
}

/// Everything the platform layer owns for the lifetime of the process.
///
/// Held in one struct rather than as globals so that the glue's userData
/// pointer is the only piece of shared state, and so ownership is obvious when
/// you come to change this file.
///
/// Member order is construction order, and it matters here: the asset manager
/// holds a reference to the file system, and the runtime holds references to
/// all three services, so each must be declared after what it refers to. The
/// application comes last, so everything it is handed already exists.
struct Platform {
    explicit Platform(android_app* native)
        : files(*native->activity->assetManager, storage_roots(native))
        , assets(files)
        , runtime{graphics, files, assets} {}

    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    sq::graphics::Context graphics;
    sq::files::AndroidAssetFileSystem files;
    sq::assets::AssetManager assets;
    sq::app::Runtime runtime;
    {{project_name}}::App app;

    bool has_focus{false};
    bool surface_ready{false};
    bool created{false};

    /// Set once the application has asked to quit and ANativeActivity_finish
    /// has been called. From then on nothing is drawn, and the loop only waits
    /// for Android to tear the activity down.
    bool finishing{false};
    Clock::time_point last_frame{Clock::now()};

    /// The drawable size the application was last told about.
    int reported_width{0};
    int reported_height{0};

    void send(const Event& event) { app.handle_event(event); }

    /// Re-measure the surface, and tell the application if its size changed.
    ///
    /// Called on every frame, not only when Android says the window changed.
    /// On rotation Android sends WINDOW_RESIZED and CONFIG_CHANGED before the
    /// surface has its new size - EGL still reports the old one until a
    /// buffer has been swapped at the new size. Measuring only on those
    /// commands caught the stale size and kept it: the viewport stayed at the
    /// previous orientation and the picture landed in part of the screen.
    ///
    /// Cost per frame: two eglQuerySurface calls and a glViewport. Nothing is
    /// allocated, and the application hears about a size only when it changes.
    void sync_size(bool always) {
        graphics.refresh_viewport();
        const int width = graphics.pixel_width();
        const int height = graphics.pixel_height();
        if (!always && width == reported_width && height == reported_height) {
            return;
        }
        reported_width = width;
        reported_height = height;
        app.resize(width, height);

        Event resized;
        resized.type = Event::Type::Resize;
        resized.width = width;
        resized.height = height;
        send(resized);
    }

    /// Bring the rendering surface up. Called when Android hands us a window,
    /// which happens more than once in a process — on rotation, and after the
    /// app returns from the background. The window is a different one each
    /// time, which is why set_native_window() exists.
    void attach(ANativeWindow* window) {
        graphics.set_native_window(window);

        const bool ready = created
            ? graphics.resume()
            : graphics.create(sq::graphics::ContextConfig{
                  .native_window = window,
                  .title = kTag,
                  .logical_width = ANativeWindow_getWidth(window),
                  .logical_height = ANativeWindow_getHeight(window)});

        if (!ready) {
            SQ_LOGW("graphics: surface not available");
            surface_ready = false;
            return;
        }

        surface_ready = true;
        SQ_LOGI("graphics: %dx%d, generation %llu, resources %s",
                graphics.pixel_width(), graphics.pixel_height(),
                static_cast<unsigned long long>(graphics.generation()),
                graphics.resources_preserved() ? "preserved" : "lost");

        if (!created) {
            if (!app.create(runtime)) {
                SQ_LOGW("app: create() failed");
                surface_ready = false;
                return;
            }
            created = true;
            last_frame = Clock::now();
        }

        app.surface_created(graphics);

        // Always, even at an unchanged size: a new surface is a new
        // application state, and the application re-lays out from here.
        sync_size(true);
    }

    /// The window is going away. Suspend rather than destroy: the context and
    /// its GPU objects often survive, and resume() is what finds out.
    void detach() {
        if (surface_ready) app.surface_destroyed();
        graphics.suspend();
        surface_ready = false;
    }

    /// One frame. Does nothing without a surface, which is the normal state
    /// while backgrounded — drawing then is wasted work at best.
    void frame() {
        if (!surface_ready) return;

        const Clock::time_point now = Clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now - last_frame);
        last_frame = now;

        // A frame after a long pause would otherwise hand the simulation a
        // delta measured in seconds, and anything integrating over it jumps.
        constexpr auto maximum_delta = std::chrono::milliseconds(100);
        if (delta > maximum_delta) delta = maximum_delta;

        sync_size(false);
        app.update(delta);
        app.render(graphics);

        // present() returns false when the surface has been lost, which is
        // normal rather than exceptional: it happens every time the app is
        // backgrounded. The only correct response is to stop drawing and wait
        // for a new window. Ignoring it spins, rendering into nothing, until
        // Android kills the process.
        if (!graphics.present()) {
            SQ_LOGI("graphics: surface lost during present");
            app.surface_destroyed();
            surface_ready = false;
        }
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
            // Rotation changes the drawable size without destroying the
            // window, so the surface has to be re-measured even though it is
            // still valid.
            // The size may not have changed yet; frame() re-measures every
            // frame and catches it when it does.
            if (platform.surface_ready) platform.sync_size(false);
            break;

        case APP_CMD_GAINED_FOCUS: {
            platform.has_focus = true;
            platform.app.resume();
            // The clock stopped meaning anything while backgrounded.
            platform.last_frame = Clock::now();
            Event event;
            event.type = Event::Type::Resume;
            platform.send(event);
            break;
        }

        case APP_CMD_LOST_FOCUS: {
            platform.has_focus = false;
            platform.app.pause();
            Event event;
            event.type = Event::Type::Pause;
            platform.send(event);
            break;
        }

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

int32_t on_motion(Platform& platform, AInputEvent* input) {
    const std::int32_t action =
        AMotionEvent_getAction(input) & AMOTION_EVENT_ACTION_MASK;
    const std::size_t index = static_cast<std::size_t>(
        (AMotionEvent_getAction(input) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
        >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);

    Event event;
    event.x = AMotionEvent_getX(input, index);
    event.y = AMotionEvent_getY(input, index);
    // The pointer id, not the index: an index is a slot in this event and is
    // reused as fingers lift, while the id follows one finger from down to up.
    // Tracking by index is how multi-touch drags end up on the wrong widget.
    event.pointer_id = AMotionEvent_getPointerId(input, index);
    event.input_device_id = AInputEvent_getDeviceId(input);

    switch (action) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            event.type = Event::Type::PointerDown;
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            event.type = Event::Type::PointerMove;
            break;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
        case AMOTION_EVENT_ACTION_CANCEL:
            event.type = Event::Type::PointerUp;
            break;
        default:
            return 0;
    }

    // A MOVE carries every active pointer, not just one. Sending only the
    // first loses the others, and a two-finger gesture then reads as a
    // one-finger drag.
    if (event.type == Event::Type::PointerMove) {
        const std::size_t count = AMotionEvent_getPointerCount(input);
        for (std::size_t i = 0; i < count; ++i) {
            Event moved = event;
            moved.x = AMotionEvent_getX(input, i);
            moved.y = AMotionEvent_getY(input, i);
            moved.pointer_id = AMotionEvent_getPointerId(input, i);
            platform.send(moved);
        }
        return 1;
    }

    platform.send(event);
    return 1;
}

int32_t on_key(Platform& platform, AInputEvent* input) {
    const std::int32_t code = AKeyEvent_getKeyCode(input);
    const std::int32_t action = AKeyEvent_getAction(input);

    Event event;
    event.key = translate_key(code);
    event.modifiers = translate_modifiers(AKeyEvent_getMetaState(input));
    event.input_device_id = code;   // the raw code, for keys squared does not name
    event.repeat = AKeyEvent_getRepeatCount(input) > 0;

    if (action == AKEY_EVENT_ACTION_DOWN) {
        event.type = Event::Type::KeyDown;
    } else if (action == AKEY_EVENT_ACTION_UP) {
        event.type = Event::Type::KeyUp;
    } else {
        return 0;
    }

    platform.send(event);

    // Back is a navigation action and also a request to leave. Send both and
    // let the application decide: a dialog consumes it, a main screen exits.
    const Event::Navigation navigation = translate_navigation(code);
    if (navigation != Event::Navigation::unknown
        && event.type == Event::Type::KeyDown) {
        Event nav;
        nav.type = Event::Type::NavigationInput;
        nav.navigation = navigation;
        nav.modifiers = event.modifiers;
        platform.send(nav);
    }

    if (code == AKEYCODE_BACK && action == AKEY_EVENT_ACTION_UP) {
        Event back;
        back.type = Event::Type::BackRequested;
        platform.send(back);
    }

    // Returning 1 for BACK stops Android closing the activity behind our back;
    // the application decides, through quit_requested(), and android_main
    // then finishes the activity.
    return 1;
}

int32_t on_input(android_app* app, AInputEvent* input) {
    Platform& platform = platform_of(app);

    switch (AInputEvent_getType(input)) {
        case AINPUT_EVENT_TYPE_MOTION: return on_motion(platform, input);
        case AINPUT_EVENT_TYPE_KEY:    return on_key(platform, input);
        default:                       return 0;
    }
}

}  // namespace

/// The glue's entry point. Named by android_native_app_glue, not by us.
void android_main(android_app* app) {
    SQ_LOGI("starting");

    // Heap-allocated because the glue's userData is a void*, and because
    // Platform holds the graphics context, which must be destroyed before this
    // function returns.
    auto platform = std::make_unique<Platform>(app);
    app->userData = platform.get();
    app->onAppCmd = on_command;
    app->onInputEvent = on_input;

    // Releases everything the platform owns, once, on the way out. One place
    // rather than two copies in the loop below, so they cannot drift apart.
    const auto shut_down = [&] {
        SQ_LOGI("shutting down");
        platform->detach();
        platform->app.dispose();
        app->userData = nullptr;
    };

    while (true) {
        int events = 0;
        android_poll_source* source = nullptr;

        // Quitting does not happen by returning from android_main. That only
        // ends this thread: the activity stays on screen, frozen on its last
        // frame, and the back button appears to have done nothing.
        //
        // ANativeActivity_finish asks Android to close the activity. Android
        // then runs the ordinary teardown - pause, window gone, stop, destroy -
        // and the glue reports destroyRequested at the end of it. Only then is
        // it safe to return. Called once; finishing guards it.
        if (!platform->finishing && platform->app.quit_requested()) {
            SQ_LOGI("finishing");
            platform->finishing = true;
            ANativeActivity_finish(app->activity);
        }

        // Block only when there is nothing to draw. With a live surface we
        // poll with a zero timeout and render continuously; without one - or
        // once finishing - we wait, so a closing or backgrounded app costs no
        // battery.
        const int timeout =
            platform->surface_ready && !platform->finishing ? 0 : -1;

        // The destroy check sits outside the poll loop as well as inside it.
        // Inside alone is enough while ALooper_pollOnce blocks on a -1 timeout,
        // which it does on a device - but if it ever returns immediately with
        // nothing to report, an inside-only check never runs and the outer
        // loop spins at full CPU without ever noticing a destroy request.
        if (app->destroyRequested != 0) {
            shut_down();
            return;
        }

        while (ALooper_pollOnce(timeout, nullptr, &events,
                                reinterpret_cast<void**>(&source)) >= 0) {
            if (source != nullptr) source->process(app, source);

            if (app->destroyRequested != 0) {
                shut_down();
                return;
            }
            if (platform->surface_ready && !platform->finishing) break;
        }

        // Nothing is drawn once finishing: the application has said it is
        // done, and its last frame should be the one it chose.
        if (!platform->finishing) platform->frame();
    }
}
