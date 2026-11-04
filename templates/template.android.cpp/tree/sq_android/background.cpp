// {{project_name}} — running in the background (see background.hpp).
//
// This file is YOURS (ownership class: seeded), like entry.cpp.
//
// Every Java call here is made on Android's UI thread: from apply (a job the
// glue thread posts) and from the service's natives, which Android calls
// there. Each opens a local frame and closes it, so no local reference
// outlives the call that made it, in a looper callback as in a native method.

#include "background.hpp"

#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>

namespace sq_android {

namespace {

constexpr const char* kService = "dev.squared.ModelService";
constexpr const char* kActionStart = "dev.squared.background.START";
constexpr const char* kActionExit = "dev.squared.background.EXIT";
constexpr const char* kActionWake = "dev.squared.background.WAKE";
constexpr const char* kChannel = "squared.background";
constexpr jint kNotificationId = 0x5351;  // "SQ"
constexpr jint kStartNotSticky = 2;       // Service.START_NOT_STICKY
constexpr jint kImportanceLow = 2;        // NotificationManager.IMPORTANCE_LOW
constexpr jint kPartialWakeLock = 1;      // PowerManager.PARTIAL_WAKE_LOCK
constexpr jint kFlagUpdateCurrent = 0x08000000;
constexpr jint kFlagImmutable = 0x04000000;  // API 23
constexpr int kForegroundServiceApi = 26;    // startForegroundService, channels
constexpr int kImmutableApi = 23;
constexpr jint kFrame = 64;

bool failed(JNIEnv* env) noexcept {
    if (env->ExceptionCheck() == JNI_FALSE) return false;
    env->ExceptionClear();
    return true;
}

/// `text`, cut to fit `out` with its terminator, at a character.
std::uint8_t keep(std::string_view text, std::array<char, 64>& out) noexcept {
    std::size_t n = std::min(text.size(), out.size() - 1);
    while (n > 0 && n < text.size() && (static_cast<unsigned char>(text[n]) & 0xC0U) == 0x80U) {
        --n;  // not inside a character
    }
    std::copy_n(text.data(), n, out.data());
    out[n] = '\0';
    return static_cast<std::uint8_t>(n);
}

jmethodID method(JNIEnv* env, jclass type, const char* name, const char* signature) noexcept {
    if (type == nullptr) return nullptr;
    jmethodID id = env->GetMethodID(type, name, signature);
    return failed(env) ? nullptr : id;
}

jmethodID static_method(JNIEnv* env, jclass type, const char* name,
                        const char* signature) noexcept {
    if (type == nullptr) return nullptr;
    jmethodID id = env->GetStaticMethodID(type, name, signature);
    return failed(env) ? nullptr : id;
}

jclass find(JNIEnv* env, const char* name) noexcept {
    jclass type = env->FindClass(name);
    return failed(env) ? nullptr : type;
}

/// context.getSystemService(name): a local reference, or nullptr.
jobject system_service(JNIEnv* env, jobject context, const char* name) noexcept {
    jmethodID get = method(env, env->GetObjectClass(context), "getSystemService",
                           "(Ljava/lang/String;)Ljava/lang/Object;");
    if (get == nullptr) return nullptr;
    jobject service = env->CallObjectMethod(context, get, env->NewStringUTF(name));
    return failed(env) ? nullptr : service;
}

}  // namespace

Background& background() noexcept {
    static Background one;
    return one;
}

void Background::say(const char* line) noexcept {
    __android_log_print(ANDROID_LOG_INFO, "squared", "%s", line);
    const std::lock_guard lock(log_mutex_);
    if (line_count_ == lines_.size()) {
        first_line_ = static_cast<std::uint8_t>((first_line_ + 1U) % lines_.size());
        --line_count_;
    }
    const std::size_t at = (first_line_ + line_count_) % lines_.size();
    std::snprintf(lines_[at].data(), lines_[at].size(), "%s", line);
    ++line_count_;
}

void Background::drain_log(Log out) noexcept {
    std::array<char, 96> line{};
    for (;;) {
        {
            const std::lock_guard lock(log_mutex_);
            if (line_count_ == 0) return;
            line = lines_[first_line_];
            first_line_ = static_cast<std::uint8_t>((first_line_ + 1U) % lines_.size());
            --line_count_;
        }
        out(line.data());
    }
}

bool Background::attach(ANativeActivity* activity) noexcept {
    if (service_class_ != nullptr) return true;
    if (vm_ != nullptr) return false;  // looked for once already: no shim
    vm_ = activity->vm;
    api_level_ = activity->sdkVersion;
    JNIEnv* env = activity->env;
    if (env->PushLocalFrame(kFrame) != JNI_OK) return false;
    bool found = false;
    jclass activity_class = env->GetObjectClass(activity->clazz);
    // The app's class loader: FindClass here would use NativeActivity's
    // (the system's), which has never heard of the shim.
    jmethodID get_loader =
        method(env, activity_class, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject loader =
        get_loader != nullptr ? env->CallObjectMethod(activity->clazz, get_loader) : nullptr;
    jclass loader_class = find(env, "java/lang/ClassLoader");
    jmethodID load =
        method(env, loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (!failed(env) && loader != nullptr && load != nullptr) {
        auto shim =
            static_cast<jclass>(env->CallObjectMethod(loader, load, env->NewStringUTF(kService)));
        if (!failed(env) && shim != nullptr) {
            // (No double braces: the generator reads them as a placeholder.)
            const std::array<JNINativeMethod, 3> natives = {
                JNINativeMethod{"onStartCommand", "(Landroid/content/Intent;II)I",
                                reinterpret_cast<void*>(&Background::on_start_command)},
                JNINativeMethod{"onBind", "(Landroid/content/Intent;)Landroid/os/IBinder;",
                                reinterpret_cast<void*>(&Background::on_bind)},
                JNINativeMethod{"onDestroy", "()V",
                                reinterpret_cast<void*>(&Background::on_destroy)},
            };
            jmethodID get_application =
                method(env, activity_class, "getApplicationContext", "()Landroid/content/Context;");
            jobject application = get_application != nullptr
                                      ? env->CallObjectMethod(activity->clazz, get_application)
                                      : nullptr;
            if (!failed(env) && application != nullptr &&
                env->RegisterNatives(shim, natives.data(), static_cast<jint>(natives.size())) ==
                    JNI_OK) {
                service_class_ = static_cast<jclass>(env->NewGlobalRef(shim));
                context_ = env->NewGlobalRef(application);
                found = true;
            }
            failed(env);
        }
    }
    env->PopLocalFrame(nullptr);
    available_.store(found, std::memory_order_release);
    say(found ? "background: dev.squared.ModelService found, natives registered"
              : "background: no dev.squared.ModelService in this APK (BACKGROUND=1 packs it)");
    return found;
}

void Background::set_glue(ALooper* looper, Post post, void* context) noexcept {
    const std::lock_guard lock(glue_mutex_);
    if (glue_ != nullptr) ALooper_release(glue_);
    glue_ = looper;
    if (glue_ != nullptr) ALooper_acquire(glue_);
    post_ = post;
    post_context_ = context;
}

bool Background::post() noexcept {
    const std::lock_guard lock(glue_mutex_);
    return post_ != nullptr && post_(post_context_);
}

bool Background::keep_running(std::string_view title, std::string_view text) {
    if (!available()) return false;
    {
        const std::lock_guard lock(asked_mutex_);
        title_size_ = keep(title, title_);
        text_size_ = keep(text, text_);
        want_running_ = true;
    }
    return post();
}

void Background::stop_running() {
    {
        const std::lock_guard lock(asked_mutex_);
        if (!want_running_) return;
        want_running_ = false;
    }
    static_cast<void>(post());
}

jobject Background::service_intent(JNIEnv* env, const char* action) noexcept {
    jclass intent_class = find(env, "android/content/Intent");
    jmethodID make =
        method(env, intent_class, "<init>", "(Landroid/content/Context;Ljava/lang/Class;)V");
    jmethodID set_action =
        method(env, intent_class, "setAction", "(Ljava/lang/String;)Landroid/content/Intent;");
    if (make == nullptr || set_action == nullptr) return nullptr;
    jobject intent = env->NewObject(intent_class, make, context_, service_class_);
    if (failed(env) || intent == nullptr) return nullptr;
    env->CallObjectMethod(intent, set_action, env->NewStringUTF(action));
    return failed(env) ? nullptr : intent;
}

void Background::apply(JNIEnv* env) noexcept {
    if (!available()) return;
    bool want = false;
    {
        const std::lock_guard lock(asked_mutex_);
        want = want_running_;
    }
    if (env->PushLocalFrame(kFrame) != JNI_OK) return;
    jclass context_class = find(env, "android/content/Context");
    jobject intent = service_intent(env, kActionStart);
    if (want && intent != nullptr) {
        // Each time: onStartCommand then shows what was asked for now. Called
        // while the app is on screen, as Android 12 requires of a start.
        const char* start =
            api_level_ >= kForegroundServiceApi ? "startForegroundService" : "startService";
        jmethodID call = method(env, context_class, start,
                                "(Landroid/content/Intent;)Landroid/content/ComponentName;");
        if (call != nullptr) env->CallObjectMethod(context_, call, intent);
        if (failed(env)) {
            say("background: Android refused to start the service");
        } else {
            if (!started_) say("background: service started");
            started_ = true;
        }
    } else if (!want && started_ && intent != nullptr) {
        set_wake_lock(env, false);
        jmethodID stop = method(env, context_class, "stopService", "(Landroid/content/Intent;)Z");
        if (stop != nullptr) env->CallBooleanMethod(context_, stop, intent);
        failed(env);
        started_ = false;
        say("background: service stopped");
    }
    env->PopLocalFrame(nullptr);
}

void Background::show(JNIEnv* env) noexcept {
    if (service_ == nullptr || env->PushLocalFrame(kFrame) != JNI_OK) return;
    std::array<char, 64> title{};
    std::array<char, 96> text{};
    {
        const std::lock_guard lock(asked_mutex_);
        std::copy_n(title_.data(), title_size_ + 1U, title.data());
        std::snprintf(text.data(), text.size(), "%s%s", text_.data(),
                      wake_held_ ? (text_size_ != 0 ? " (wake lock held)" : "Wake lock held") : "");
    }
    jobject context = service_;
    jclass context_class = env->GetObjectClass(context);
    // The channel, once (Android 8 and later). Android 13 asks for the
    // notification permission when an app targeting Android 12 or earlier
    // makes its first, while it is on screen.
    if (api_level_ >= kForegroundServiceApi && !channel_made_) {
        jclass channel_class = find(env, "android/app/NotificationChannel");
        jmethodID make =
            method(env, channel_class, "<init>", "(Ljava/lang/String;Ljava/lang/CharSequence;I)V");
        jobject manager = system_service(env, context, "notification");
        jmethodID create = manager != nullptr ? method(env, env->GetObjectClass(manager),
                                                       "createNotificationChannel",
                                                       "(Landroid/app/NotificationChannel;)V")
                                              : nullptr;
        if (make != nullptr && create != nullptr) {
            jobject channel =
                env->NewObject(channel_class, make, env->NewStringUTF(kChannel),
                               env->NewStringUTF("Running in the background"), kImportanceLow);
            if (!failed(env) && channel != nullptr) {
                env->CallVoidMethod(manager, create, channel);
                channel_made_ = !failed(env);
            }
        }
    }
    jclass builder_class = find(env, "android/app/Notification$Builder");
    jobject builder = nullptr;
    if (builder_class != nullptr) {
        if (api_level_ >= kForegroundServiceApi) {
            jmethodID make = method(env, builder_class, "<init>",
                                    "(Landroid/content/Context;Ljava/lang/String;)V");
            if (make != nullptr)
                builder = env->NewObject(builder_class, make, context, env->NewStringUTF(kChannel));
        } else {
            jmethodID make = method(env, builder_class, "<init>", "(Landroid/content/Context;)V");
            if (make != nullptr) builder = env->NewObject(builder_class, make, context);
        }
        if (failed(env)) builder = nullptr;
    }
    // The app's icon, as the small icon (Android draws it as a silhouette).
    jint icon = 0;
    jmethodID get_info =
        method(env, context_class, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
    jobject info = get_info != nullptr ? env->CallObjectMethod(context, get_info) : nullptr;
    if (!failed(env) && info != nullptr) {
        jfieldID icon_field = env->GetFieldID(env->GetObjectClass(info), "icon", "I");
        if (!failed(env) && icon_field != nullptr) icon = env->GetIntField(info, icon_field);
    }
    const jint flags = kFlagUpdateCurrent | (api_level_ >= kImmutableApi ? kFlagImmutable : 0);
    jclass pending_class = find(env, "android/app/PendingIntent");
    const char* pending_signature =
        "(Landroid/content/Context;ILandroid/content/Intent;I)Landroid/app/PendingIntent;";
    jmethodID for_activity = static_method(env, pending_class, "getActivity", pending_signature);
    jmethodID for_service = static_method(env, pending_class, "getService", pending_signature);
    // Tapped: the app, as its launcher icon opens it.
    jobject open = nullptr;
    jmethodID get_manager =
        method(env, context_class, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jmethodID get_name = method(env, context_class, "getPackageName", "()Ljava/lang/String;");
    if (get_manager != nullptr && get_name != nullptr && for_activity != nullptr) {
        jobject manager = env->CallObjectMethod(context, get_manager);
        jobject name = env->CallObjectMethod(context, get_name);
        jmethodID launch =
            !failed(env) && manager != nullptr
                ? method(env, env->GetObjectClass(manager), "getLaunchIntentForPackage",
                         "(Ljava/lang/String;)Landroid/content/Intent;")
                : nullptr;
        jobject intent = launch != nullptr ? env->CallObjectMethod(manager, launch, name) : nullptr;
        if (!failed(env) && intent != nullptr) {
            open =
                env->CallStaticObjectMethod(pending_class, for_activity, context, 0, intent, flags);
            if (failed(env)) open = nullptr;
        }
    }
    const auto action = [&](const char* name, jint code) -> jobject {
        jobject intent = service_intent(env, name);
        if (intent == nullptr || for_service == nullptr) return nullptr;
        jobject pending =
            env->CallStaticObjectMethod(pending_class, for_service, context, code, intent, flags);
        return failed(env) ? nullptr : pending;
    };
    jobject exit_action = action(kActionExit, 1);
    jobject wake_action = action(kActionWake, 2);
    const char* returns_builder = "Landroid/app/Notification$Builder;";
    const auto signature = [&](const char* parameters, std::array<char, 160>& out) {
        std::snprintf(out.data(), out.size(), "(%s)%s", parameters, returns_builder);
        return out.data();
    };
    std::array<char, 160> sig{};
    if (builder != nullptr) {
        jmethodID set_icon = method(env, builder_class, "setSmallIcon", signature("I", sig));
        jmethodID set_title = method(env, builder_class, "setContentTitle",
                                     signature("Ljava/lang/CharSequence;", sig));
        jmethodID set_text = method(env, builder_class, "setContentText",
                                    signature("Ljava/lang/CharSequence;", sig));
        jmethodID set_ongoing = method(env, builder_class, "setOngoing", signature("Z", sig));
        jmethodID set_intent = method(env, builder_class, "setContentIntent",
                                      signature("Landroid/app/PendingIntent;", sig));
        jmethodID add_action =
            method(env, builder_class, "addAction",
                   signature("ILjava/lang/CharSequence;Landroid/app/PendingIntent;", sig));
        jmethodID build = method(env, builder_class, "build", "()Landroid/app/Notification;");
        jmethodID start =
            method(env, context_class, "startForeground", "(ILandroid/app/Notification;)V");
        if (set_icon != nullptr && set_title != nullptr && set_text != nullptr &&
            set_ongoing != nullptr && build != nullptr && start != nullptr) {
            env->CallObjectMethod(builder, set_icon, icon);
            env->CallObjectMethod(builder, set_title, env->NewStringUTF(title.data()));
            env->CallObjectMethod(builder, set_text, env->NewStringUTF(text.data()));
            env->CallObjectMethod(builder, set_ongoing, JNI_TRUE);
            if (open != nullptr && set_intent != nullptr)
                env->CallObjectMethod(builder, set_intent, open);
            if (add_action != nullptr && exit_action != nullptr) {
                env->CallObjectMethod(builder, add_action, 0, env->NewStringUTF("Exit"),
                                      exit_action);
            }
            if (add_action != nullptr && wake_action != nullptr) {
                env->CallObjectMethod(
                    builder, add_action, 0,
                    env->NewStringUTF(wake_held_ ? "Release wakelock" : "Acquire wakelock"),
                    wake_action);
            }
            jobject notification = failed(env) ? nullptr : env->CallObjectMethod(builder, build);
            if (!failed(env) && notification != nullptr) {
                env->CallVoidMethod(context, start, kNotificationId, notification);
            }
            if (failed(env)) say("background: the notification could not be shown");
        }
    }
    env->PopLocalFrame(nullptr);
}

void Background::set_wake_lock(JNIEnv* env, bool held) noexcept {
    if (held == wake_held_ || env->PushLocalFrame(kFrame) != JNI_OK) return;
    if (held && wake_lock_ == nullptr) {
        jobject power = system_service(env, context_, "power");
        jmethodID make = power != nullptr
                             ? method(env, env->GetObjectClass(power), "newWakeLock",
                                      "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;")
                             : nullptr;
        jobject lock = make != nullptr
                           ? env->CallObjectMethod(power, make, kPartialWakeLock,
                                                   env->NewStringUTF("squared:background"))
                           : nullptr;
        if (!failed(env) && lock != nullptr) {
            jmethodID counted =
                method(env, env->GetObjectClass(lock), "setReferenceCounted", "(Z)V");
            if (counted != nullptr) env->CallVoidMethod(lock, counted, JNI_FALSE);
            failed(env);
            wake_lock_ = env->NewGlobalRef(lock);
        }
    }
    if (wake_lock_ != nullptr) {
        jmethodID call =
            method(env, env->GetObjectClass(wake_lock_), held ? "acquire" : "release", "()V");
        if (call != nullptr) env->CallVoidMethod(wake_lock_, call);
        if (!failed(env)) {
            wake_held_ = held;
            say(held ? "background: wake lock held" : "background: wake lock released");
        }
    }
    env->PopLocalFrame(nullptr);
}

void Background::exit(JNIEnv* env) noexcept {
    {
        // An activity runs: the app quits there (QuitRequested on the glue
        // thread), and quitting stops the service.
        const std::lock_guard lock(glue_mutex_);
        if (glue_ != nullptr) {
            exit_asked_.store(true);
            ALooper_wake(glue_);
            return;
        }
    }
    // None: nothing runs the app's code but its model. Stop the service and
    // end the process; the model saved when the app left the screen.
    say("background: Exit with no activity: the service stops, the process ends");
    set_wake_lock(env, false);
    if (service_ != nullptr && env->PushLocalFrame(kFrame) == JNI_OK) {
        jclass type = env->GetObjectClass(service_);
        jmethodID stop_foreground = method(env, type, "stopForeground", "(Z)V");
        jmethodID stop_self = method(env, type, "stopSelf", "()V");
        if (stop_foreground != nullptr) env->CallVoidMethod(service_, stop_foreground, JNI_TRUE);
        if (stop_self != nullptr) env->CallVoidMethod(service_, stop_self);
        failed(env);
        env->PopLocalFrame(nullptr);
    }
    std::_Exit(0);
}

jint Background::on_start_command(JNIEnv* env, jobject service, jobject intent, jint /*flags*/,
                                  jint /*id*/) noexcept {
    Background& self = background();
    if (self.service_ == nullptr) self.service_ = env->NewGlobalRef(service);
    std::array<char, 64> action{};
    if (intent != nullptr && env->PushLocalFrame(kFrame) == JNI_OK) {
        jmethodID get_action =
            method(env, env->GetObjectClass(intent), "getAction", "()Ljava/lang/String;");
        auto name = get_action != nullptr
                        ? static_cast<jstring>(env->CallObjectMethod(intent, get_action))
                        : nullptr;
        if (!failed(env) && name != nullptr) {
            const char* chars = env->GetStringUTFChars(name, nullptr);
            if (chars != nullptr) {
                std::snprintf(action.data(), action.size(), "%s", chars);
                env->ReleaseStringUTFChars(name, chars);
            }
        }
        env->PopLocalFrame(nullptr);
    }
    const std::string_view asked(action.data());
    if (asked == kActionExit) {
        self.exit(env);
        return kStartNotSticky;
    }
    if (asked == kActionWake) {
        self.set_wake_lock(env, !self.wake_held_);
    }
    // Shown every time: Android 8 and later end an app that starts a
    // foreground service and does not show its notification within seconds.
    self.show(env);
    bool want = false;
    {
        const std::lock_guard lock(self.asked_mutex_);
        want = self.want_running_;
    }
    if (!want && env->PushLocalFrame(kFrame) == JNI_OK) {
        // Stopped while this start was on its way.
        jmethodID stop_self = method(env, env->GetObjectClass(service), "stopSelf", "()V");
        if (stop_self != nullptr) env->CallVoidMethod(service, stop_self);
        failed(env);
        env->PopLocalFrame(nullptr);
    }
    return kStartNotSticky;
}

jobject Background::on_bind(JNIEnv* /*env*/, jobject /*service*/, jobject /*intent*/) noexcept {
    return nullptr;  // started, never bound
}

void Background::on_destroy(JNIEnv* env, jobject /*service*/) noexcept {
    Background& self = background();
    self.set_wake_lock(env, false);
    if (self.service_ != nullptr) {
        env->DeleteGlobalRef(self.service_);
        self.service_ = nullptr;
    }
    self.started_ = false;
}

}  // namespace sq_android
