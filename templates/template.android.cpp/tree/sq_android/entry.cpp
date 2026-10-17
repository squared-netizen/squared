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
//   * measures the system bars and camera cut-out and tells the application
//     (sq::app::SafeArea), because Android draws the app edge to edge
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
#include <squared/app/safe_area.hpp>
#include <squared/app/src/key.hpp>
#include <squared/app/src/key_modifiers.hpp>
#include <squared/app/src/navigation.hpp>
#include <squared/graphics/context.hpp>
#include <squared/graphics/src/context_config.hpp>

#include <android/configuration.h>
#include <android/log.h>
#include <android/keycodes.h>
#include <android_native_app_glue.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <jni.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>

// Development builds only (`make DEV_LOG=1`): every log line below is also sent
// to a UDP port on this device, a fatal signal sends a backtrace, and frames
// whose update() or render() allocate are reported. The value of
// SQUARED_DEV_LOG is the port. Nothing of this exists in a normal build.
#ifdef SQUARED_DEV_LOG
#include <squared/dev/dev_log.hpp>
#include <squared/dev/frame_allocations.hpp>
#endif

// The in-app Console (`make CONSOLE=app`): drawn over the application's frame,
// with every log line below mirrored into it. Nothing of it exists in a
// CONSOLE=0 build. For now it only shows: the keyboard and the gesture that
// opens it come later (Sprint 4.5), so it is open from the start and its [x]
// closes it for the rest of the run.
#ifdef SQUARED_CONSOLE
#include <squared/console/console.hpp>
#include <squared/console/layout.hpp>
#include <squared/console/renderer.hpp>
#endif

#if defined(SQUARED_DEV_LOG) || defined(SQUARED_CONSOLE)
#include <cstdarg>
#include <cstdio>
#include <string_view>
#endif

namespace {

constexpr const char* kTag = "{{project_name}}";

#ifdef SQUARED_CONSOLE
/// The Console log lines are mirrored into: the platform's, while it exists.
/// Lines logged before (the first one) or after are not shown in it.
sq::console::Console* console_sink = nullptr;
#endif

#if defined(SQUARED_DEV_LOG) || defined(SQUARED_CONSOLE)
/// Format a log line once, into the stack, and hand it to the dev log and the
/// Console, whichever this build has.
__attribute__((format(printf, 2, 3))) void mirror(int priority, const char* format, ...) {
    char line[512];
    va_list arguments;
    va_start(arguments, format);
    const int length = std::vsnprintf(line, sizeof(line), format, arguments);
    va_end(arguments);
    if (length <= 0) return;
    const std::string_view text{line, static_cast<std::size_t>(length) < sizeof(line)
                                          ? static_cast<std::size_t>(length)
                                          : sizeof(line) - 1};
#ifdef SQUARED_DEV_LOG
    sq::dev::log_line(text);
#endif
#ifdef SQUARED_CONSOLE
    if (console_sink != nullptr) {
        console_sink->print(text, priority >= ANDROID_LOG_WARN ? sq::console::Level::warning
                                                                : sq::console::Level::info);
    }
#else
    static_cast<void>(priority);
#endif
}
#define SQ_MIRROR(priority, ...) mirror(priority, __VA_ARGS__)
#else
#define SQ_MIRROR(...) ((void)0)
#endif

#define SQ_LOGI(...) \
    (__android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__), \
     SQ_MIRROR(ANDROID_LOG_INFO, __VA_ARGS__))
#define SQ_LOGW(...) \
    (__android_log_print(ANDROID_LOG_WARN, kTag, __VA_ARGS__), \
     SQ_MIRROR(ANDROID_LOG_WARN, __VA_ARGS__))

#ifdef SQUARED_DEV_LOG
// Runs when the library loads, before any static initializer of ours, so a
// crash while the framework is starting up is still reported. Priority 101 is
// the earliest user-level slot.
__attribute__((constructor(101))) void open_dev_log() {
    const auto status = sq::dev::open_log_sink({.port = SQUARED_DEV_LOG});
    if (status == sq::dev::LogSinkStatus::Opened) {
        SQ_LOGI("dev log: sending to 127.0.0.1:%d", static_cast<int>(SQUARED_DEV_LOG));
    } else {
        __android_log_print(ANDROID_LOG_WARN, kTag,
                            "dev log: socket refused; is INTERNET in the manifest?");
    }
}
#endif

using Event = sq::app::Event;
using Key = sq::app::Key;
using Navigation = sq::app::Navigation;
using Clock = std::chrono::steady_clock;

/// Translate an Android keycode into squared's portable key.
///
/// Only the keys squared names. Everything else arrives as Key::unknown with
/// the Android code still in input_device_id, so an application that needs a
/// key squared does not name can still find it.
Key translate_key(std::int32_t code) {
    switch (code) {
        case AKEYCODE_DPAD_LEFT:  return Key::left;
        case AKEYCODE_DPAD_RIGHT: return Key::right;
        case AKEYCODE_DPAD_UP:    return Key::up;
        case AKEYCODE_DPAD_DOWN:  return Key::down;
        case AKEYCODE_MOVE_HOME:  return Key::home;
        case AKEYCODE_MOVE_END:   return Key::end;
        case AKEYCODE_DEL:        return Key::backspace;
        case AKEYCODE_FORWARD_DEL:return Key::delete_key;
        case AKEYCODE_ENTER:
        case AKEYCODE_NUMPAD_ENTER:
        case AKEYCODE_DPAD_CENTER:return Key::enter;
        case AKEYCODE_SPACE:      return Key::space;
        case AKEYCODE_TAB:        return Key::tab;
        case AKEYCODE_ESCAPE:     return Key::escape;
        default:                  return Key::unknown;
    }
}

/// Translate an Android keycode into a semantic navigation action.
///
/// A D-pad or a game controller drives focus movement without the application
/// having to know which key did it. This is what makes a squared GUI navigable
/// on a TV or with a gamepad attached.
Navigation translate_navigation(std::int32_t code) {
    switch (code) {
        case AKEYCODE_DPAD_LEFT:  return Navigation::left;
        case AKEYCODE_DPAD_RIGHT: return Navigation::right;
        case AKEYCODE_DPAD_UP:    return Navigation::up;
        case AKEYCODE_DPAD_DOWN:  return Navigation::down;
        case AKEYCODE_TAB:        return Navigation::next;
        case AKEYCODE_DPAD_CENTER:
        case AKEYCODE_ENTER:      return Navigation::activate;
        case AKEYCODE_BACK:
        case AKEYCODE_ESCAPE:     return Navigation::cancel;
        default:                  return Navigation::unknown;
    }
}

sq::app::KeyModifiers translate_modifiers(std::int32_t state) {
    return {.shift = (state & AMETA_SHIFT_ON) != 0,
            .control = (state & AMETA_CTRL_ON) != 0,
            .alt = (state & AMETA_ALT_ON) != 0,
            .meta = (state & AMETA_META_ON) != 0};
}

// ---------------------------------------------------------------------------
// Safe area: how far the status bar, the navigation bar and a camera cut-out
// reach into the window.
//
// Since Android 15 an application draws edge to edge, so those bars sit on top
// of the surface. NativeActivity offers no call for their size, so it is asked
// of the Java side: Activity -> Window -> decor view -> root window insets.
// Every step can fail (no window yet, a method missing on an old release), and
// every failure means "nothing is covering the window" - the application then
// behaves as it did before this existed.
// ---------------------------------------------------------------------------

/// Call an object-returning method that takes no arguments. Null on any failure.
jobject call_object(JNIEnv* env, jobject target, const char* name, const char* signature) {
    if (target == nullptr) return nullptr;
    jclass type = env->GetObjectClass(target);
    jmethodID method = type != nullptr ? env->GetMethodID(type, name, signature) : nullptr;
    jobject result = method != nullptr ? env->CallObjectMethod(target, method) : nullptr;
    if (type != nullptr) env->DeleteLocalRef(type);
    if (env->ExceptionCheck() != JNI_FALSE) {
        env->ExceptionClear();
        return nullptr;
    }
    return result;
}

/// Call an int-returning method that takes no arguments. `fallback` on any failure.
jint call_int(JNIEnv* env, jobject target, const char* name, jint fallback) {
    if (target == nullptr) return fallback;
    jclass type = env->GetObjectClass(target);
    jmethodID method = type != nullptr ? env->GetMethodID(type, name, "()I") : nullptr;
    const jint result = method != nullptr ? env->CallIntMethod(target, method) : fallback;
    if (type != nullptr) env->DeleteLocalRef(type);
    if (env->ExceptionCheck() != JNI_FALSE) {
        env->ExceptionClear();
        return fallback;
    }
    return result;
}

std::int16_t to_pixels(jint value) {
    return static_cast<std::int16_t>(std::clamp<jint>(value, 0, 4096));
}

/// This process's name, such as "com.example.app" or "com.example.app:console":
/// the first string in /proc/self/cmdline, which Android sets to the process
/// name from the manifest. One open and one read, once, at start. Empty if
/// the file cannot be read.
void process_name(char* out, std::size_t size) {
    out[0] = '\0';
    const int fd = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return;
    const ssize_t got = read(fd, out, size - 1);
    close(fd);
    out[got > 0 ? static_cast<std::size_t>(got) : 0] = '\0';
}

/// The releases the safe-area query needs. View.getRootWindowInsets() arrived
/// in API 23 and WindowInsets.getDisplayCutout() in API 28. Below 23 the bars
/// sit outside the window, so there is nothing to measure; asking anyway made
/// the method lookup fail, and Java build and throw an error, on every query.
constexpr int kInsetsApi = 23;
constexpr int kCutoutApi = 28;

#ifdef SQUARED_CONSOLE
/// The Console's character cell for the screen's density: 8 pixels for each
/// 160 dpi, rounded, so the 8-pixel font is only ever scaled by whole steps.
/// A configuration without a number (default, any, none) counts as 160.
int console_cell(AConfiguration* config) {
    std::int32_t dpi = config != nullptr ? AConfiguration_getDensity(config) : 0;
    if (dpi <= 0 || dpi >= ACONFIGURATION_DENSITY_ANY) dpi = ACONFIGURATION_DENSITY_MEDIUM;
    return 8 * std::max(1, static_cast<int>((dpi + 80) / 160));
}
#endif

/// Read the insets of the activity's window, in pixels. Zero when unavailable,
/// and without a single JNI call below kInsetsApi.
///
/// The system-window insets already include a cut-out on current releases; the
/// cut-out's own safe insets are folded in with max() for the releases where
/// they do not. No allocation on our side. Each call makes a handful of JNI
/// calls and deletes every local reference it creates, so it is cheap enough
/// to repeat twice a second.
sq::app::SafeArea measure_safe_area(JNIEnv* env, jobject activity, int api_level) {
    sq::app::SafeArea area;
    if (env == nullptr || activity == nullptr || api_level < kInsetsApi) return area;

    jobject window = call_object(env, activity, "getWindow", "()Landroid/view/Window;");
    jobject decor = call_object(env, window, "getDecorView", "()Landroid/view/View;");
    jobject insets = call_object(env, decor, "getRootWindowInsets",
                                 "()Landroid/view/WindowInsets;");
    if (insets != nullptr) {
        jint left = call_int(env, insets, "getSystemWindowInsetLeft", 0);
        jint top = call_int(env, insets, "getSystemWindowInsetTop", 0);
        jint right = call_int(env, insets, "getSystemWindowInsetRight", 0);
        jint bottom = call_int(env, insets, "getSystemWindowInsetBottom", 0);

        jobject cutout = api_level >= kCutoutApi
            ? call_object(env, insets, "getDisplayCutout", "()Landroid/view/DisplayCutout;")
            : nullptr;
        if (cutout != nullptr) {
            left = std::max(left, call_int(env, cutout, "getSafeInsetLeft", 0));
            top = std::max(top, call_int(env, cutout, "getSafeInsetTop", 0));
            right = std::max(right, call_int(env, cutout, "getSafeInsetRight", 0));
            bottom = std::max(bottom, call_int(env, cutout, "getSafeInsetBottom", 0));
            env->DeleteLocalRef(cutout);
        }
        area = {to_pixels(left), to_pixels(top), to_pixels(right), to_pixels(bottom)};
        env->DeleteLocalRef(insets);
    }
    if (decor != nullptr) env->DeleteLocalRef(decor);
    if (window != nullptr) env->DeleteLocalRef(window);
    return area;
}

/// Tell an application about the safe area, if it has a use for it.
///
/// An application written before SafeArea existed has no such method; the
/// requires-expression lets it keep compiling rather than forcing an edit.
template <class Application>
void notify_safe_area(Application& application, const sq::app::SafeArea& area) {
    if constexpr (requires { application.safe_area_changed(area); }) {
        application.safe_area_changed(area);
    }
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
        , runtime{graphics, files, assets}
        , vm(native->activity->vm)
        , activity(native->activity->clazz)
        , api_level(native->config != nullptr ? AConfiguration_getSdkVersion(native->config) : 0) {
#ifdef SQUARED_CONSOLE
        console_cell_pixels = console_cell(native->config);
        console_sink = &console;
        console.print("console: the log, as it is written; [x] closes it");
#endif
    }

    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;
#ifdef SQUARED_CONSOLE
    ~Platform() { console_sink = nullptr; }
#endif

    sq::graphics::Context graphics;
    sq::files::AndroidAssetFileSystem files;
    sq::assets::AssetManager assets;
    sq::app::Runtime runtime;
    {{project_name}}::App app;

#ifdef SQUARED_CONSOLE
    /// The Console and what draws it, inside this object's one allocation
    /// (about 88 KB at the defaults). After graphics, so the renderer's GL
    /// objects are deleted before the context is.
    sq::console::Console console;
    sq::console::Renderer console_renderer;
    int console_cell_pixels{8};
    bool console_open{true};
#endif

    /// The Java side, for the safe-area query. Both are owned by Android and
    /// outlive the platform object.
    JavaVM* vm;
    jobject activity;

    /// The device's API level, read once from the configuration NativeActivity
    /// already holds. What the device lacks is decided from this, by comparing
    /// numbers, not by calling and catching the failure.
    int api_level;
    JNIEnv* env{nullptr};
    bool env_attached{false};
    sq::app::SafeArea reported_area{};
    int frames_since_measure{0};
    int fast_measure_frames{0};

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

    /// The JNI environment for this thread. Attached once on first use and
    /// kept: attaching and detaching around every query would allocate a Java
    /// thread object each time.
    JNIEnv* jni() {
        if (env != nullptr || vm == nullptr) return env;
        if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
            return env;
        }
        if (vm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            env_attached = true;
        } else {
            env = nullptr;
        }
        return env;
    }

    /// Release the attachment taken by jni(). Must run on the same thread,
    /// before android_main returns.
    void release_jni() {
        if (env_attached && vm != nullptr) vm->DetachCurrentThread();
        env = nullptr;
        env_attached = false;
    }

    /// Measure the bars and tell the application if they changed.
    ///
    /// The numbers are not available at the first frame - the window has not
    /// been laid out yet - and change on rotation, so this runs after a size
    /// change and then about twice a second. An unchanged answer costs the
    /// JNI calls and nothing else.
    void sync_safe_area() {
        frames_since_measure = 0;
        if (api_level < kInsetsApi) return;
        const sq::app::SafeArea measured = measure_safe_area(jni(), activity, api_level);
        if (measured == reported_area) return;
        reported_area = measured;
        SQ_LOGI("safe area (px): left %d top %d right %d bottom %d",
                static_cast<int>(measured.left), static_cast<int>(measured.top),
                static_cast<int>(measured.right), static_cast<int>(measured.bottom));
        notify_safe_area(app, measured);
    }

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

        // The bars are different in the other orientation, and Android reports
        // the new ones a few frames after the new size. Ask again on each of
        // the next frames rather than waiting out the slow poll, which left
        // the interface laid out for the old orientation for half a second.
        fast_measure_frames = 12;
        sync_safe_area();
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
        if (!created) log_driver();
#ifdef SQUARED_CONSOLE
        // Before the application: the Console is for when that goes wrong.
        if (!graphics.resources_preserved()) console_renderer.invalidate();
        if (!console_renderer.valid() && !console_renderer.create()) {
            SQ_LOGW("console: renderer not available");
        }
#endif

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

#ifdef SQUARED_CONSOLE
    /// Where the Console is drawn: the upper half of the screen, inside the
    /// safe area.
    sq::console::Frame console_frame() const {
        const int left = reported_area.left;
        const int top = reported_area.top;
        return {.x = left,
                .y = top,
                .width = reported_width - left - reported_area.right,
                .height = (reported_height - top - reported_area.bottom) / 2,
                .cell = console_cell_pixels};
    }
#endif

    /// Name the GL driver once, after the first context is current: what a
    /// report from a device needs, and three calls.
    ///
    /// Through dlsym rather than a direct call, so this file does not link GL
    /// itself: the graphics backend decides that, and under the null backend
    /// there is no GL to ask and the line is simply not printed.
    static void log_driver() {
        using GetString = const unsigned char* (*)(unsigned int);
        // Already loaded by the backend; NOLOAD finds it and never loads it.
        // Either name: on some releases one is a link to the other.
        void* gles = dlopen("libGLESv3.so", RTLD_NOW | RTLD_NOLOAD);
        if (gles == nullptr) gles = dlopen("libGLESv2.so", RTLD_NOW | RTLD_NOLOAD);
        if (gles == nullptr) return;
        const auto get = reinterpret_cast<GetString>(dlsym(gles, "glGetString"));
        if (get != nullptr) {
            constexpr unsigned int kVendor = 0x1F00, kRenderer = 0x1F01, kVersion = 0x1F02;
            const auto text = [&](unsigned int name) {
                const unsigned char* value = get(name);
                return value != nullptr ? reinterpret_cast<const char*>(value) : "?";
            };
            SQ_LOGI("driver: %s; %s; %s", text(kVersion), text(kRenderer), text(kVendor));
        }
        dlclose(gles);
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
        constexpr int measure_interval_frames = 30;
        if (fast_measure_frames > 0) {
            --fast_measure_frames;
            sync_safe_area();
        } else if (++frames_since_measure >= measure_interval_frames) {
            sync_safe_area();
        }
#ifdef SQUARED_DEV_LOG
        // Count the heap allocations update() and render() make. Frames that
        // allocate are reported over the log sink: the first at once, then a
        // summary every 120 frames. A frame should allocate nothing.
        sq::dev::frame_begin();
#endif
        app.update(delta);
        app.render(graphics);
#ifdef SQUARED_CONSOLE
        if (console_open) {
            console_renderer.draw(console, console_frame(), graphics.pixel_width(),
                                  graphics.pixel_height());
        }
#endif
#ifdef SQUARED_DEV_LOG
        sq::dev::frame_end();
#endif

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

#ifdef SQUARED_CONSOLE
    // The Console's [x]. Every other touch still reaches the application: the
    // Console takes input of its own in Sprint 4.5.
    if (platform.console_open && action == AMOTION_EVENT_ACTION_DOWN &&
        sq::console::touches_close(platform.console_frame(), static_cast<int>(event.x),
                                   static_cast<int>(event.y))) {
        platform.console_open = false;
        SQ_LOGI("console: closed");
        return 1;
    }
#endif

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
    const Navigation navigation = translate_navigation(code);
    if (navigation != Navigation::unknown
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
    char process[128];
    process_name(process, sizeof(process));
    SQ_LOGI("device: API %d, process %s", platform->api_level, process);
    app->onAppCmd = on_command;
    app->onInputEvent = on_input;

    // Releases everything the platform owns, once, on the way out. One place
    // rather than two copies in the loop below, so they cannot drift apart.
    const auto shut_down = [&] {
        SQ_LOGI("shutting down");
        platform->detach();
        platform->app.dispose();
        platform->release_jni();
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
