// {{project_name}} — running in the background: squared's foreground
// service, dev.squared.ModelService, and its notification and wake lock.
//
// This file is YOURS (ownership class: seeded), like entry.cpp.
//
// The service is a shim (squared's tools/dexgen: a dex with a constructor and
// three native methods, packed as classes.dex when the Makefile says
// BACKGROUND=1). Its natives are here. They are registered when the activity
// is made, through the activity's class loader (the app's), so the dex names
// no library and needs no System.loadLibrary.
//
// Threads: the App calls keep_running and stop_running on the glue thread;
// they keep what was asked for and post a job to Android's UI thread, where
// every Java call is made (apply), and where Android calls the natives. One
// object for the process: it outlives each activity, as the service does.
//
// The notification: the app's title and text, tapped to open the app; Exit
// (the app quits: QuitRequested, sent on the glue thread; with no activity
// left, the service stops and the process ends); Acquire / Release wakelock
// (PowerManager's partial wake lock: the CPU stays on with the screen off).
// Never sticky: Android does not start it again by itself, in a process
// where nothing has registered the natives.

#pragma once

#include <android/looper.h>
#include <android/native_activity.h>
#include <jni.h>

import sq.app;

import std;

namespace sq_android {

class Background final : public sq::app::BackgroundService {
public:
    /// Where a job for Android's UI thread goes: the entry point's pipe.
    using Post = bool (*)(void* context) noexcept;

    /// On Android's UI thread, as the activity is made: the service class,
    /// through the activity's class loader, its natives registered. Once a
    /// process; false (and nothing available) when the APK has no shim dex.
    bool attach(ANativeActivity* activity) noexcept;

    /// The glue thread of the activity that runs now (its looper, and how to
    /// post to the UI thread), or none (nullptr) as it ends.
    void set_glue(ALooper* looper, Post post, void* context) noexcept;

    /// The lines said since the last call, oldest first, each to `out`
    /// (the glue thread: the Console mirrors them; Android's log has them
    /// already). At most 8 wait; older ones are dropped.
    using Log = void (*)(const char* line) noexcept;
    void drain_log(Log out) noexcept;

    /// Exit was tapped in the notification: true once, on the glue thread.
    [[nodiscard]] bool take_exit() noexcept { return exit_asked_.exchange(false); }

    /// The UI thread's job: what keep_running and stop_running asked for.
    void apply(JNIEnv* env) noexcept;

    // sq::app::BackgroundService, on the glue thread.
    [[nodiscard]] bool available() const noexcept override {
        return available_.load(std::memory_order_acquire);
    }
    bool keep_running(std::string_view title, std::string_view text) override;
    void stop_running() override;

private:
    static jint on_start_command(JNIEnv* env, jobject service, jobject intent, jint flags,
                                 jint id) noexcept;
    static jobject on_bind(JNIEnv* env, jobject service, jobject intent) noexcept;
    static void on_destroy(JNIEnv* env, jobject service) noexcept;

    bool post() noexcept;
    void show(JNIEnv* env) noexcept;
    void set_wake_lock(JNIEnv* env, bool held) noexcept;
    void exit(JNIEnv* env) noexcept;
    [[nodiscard]] jobject service_intent(JNIEnv* env, const char* action) noexcept;

    /// Android's log at once; the Console's at the glue thread's next turn
    /// (the Console is that thread's alone).
    void say(const char* line) noexcept;

    // Set once, on the UI thread, by attach.
    JavaVM* vm_{nullptr};
    jobject context_{nullptr};  ///< the application's context: a global reference
    jclass service_class_{nullptr};
    int api_level_{0};
    std::atomic<bool> available_{false};

    // What the App asked for: written on the glue thread, read on the UI's.
    std::mutex asked_mutex_;
    std::array<char, 64> title_{};
    std::array<char, 64> text_{};
    std::uint8_t title_size_{0};
    std::uint8_t text_size_{0};
    bool want_running_{false};

    // The glue thread that runs now: under its own lock, since the UI thread
    // wakes it while it may be ending.
    std::mutex glue_mutex_;
    ALooper* glue_{nullptr};
    Post post_{nullptr};
    void* post_context_{nullptr};
    std::atomic<bool> exit_asked_{false};

    // Lines for the Console, said on either thread.
    std::mutex log_mutex_;
    std::array<std::array<char, 96>, 8> lines_{};
    std::uint8_t first_line_{0};
    std::uint8_t line_count_{0};

    // The UI thread's alone.
    jobject service_{nullptr};    ///< the running service: a global reference
    jobject wake_lock_{nullptr};  ///< a global reference, made at the first acquire
    bool started_{false};
    bool wake_held_{false};
    bool channel_made_{false};
};

/// The process's one.
Background& background() noexcept;

}  // namespace sq_android
