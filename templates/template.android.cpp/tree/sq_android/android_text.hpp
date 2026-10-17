// {{project_name}} — the Java side of text, for the entry point: the soft
// keyboard, what a key press types, and the clipboard.
//
// This file is YOURS (ownership class: seeded), like entry.cpp.
//
// NativeActivity hands the native side key codes, never characters, and has
// no way of its own to show the keyboard or reach the clipboard. All three
// are Java APIs, reached here through JNI from the glue thread; there is no
// Java code in the app. Classes and method ids are looked up once, in
// create(); each call after that is a handful of JNI calls, made when a person
// types or copies, never per frame.
//
// What it does not do: an input method's composition (accents built from two
// keys, Chinese and Japanese input, emoji) needs an InputConnection, which is
// Java. Keys that type ASCII and the Latin letters the keyboard maps directly
// work; the rest does not arrive.

#pragma once

#include <squared/app/src/text_input_service.hpp>

#include <android/native_activity.h>
#include <jni.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace sq_android {

/// UTF-8 for one code point, into `out`; how many bytes (0 for an invalid one).
std::size_t encode_utf8(char32_t code_point, std::span<char, 4> out) noexcept;

/// Decode UTF-8 from `text` at `at` into UTF-16 units in `out`, stopping when
/// `out` is full or the text ends; returns units written and moves `at` past
/// what was consumed. An invalid byte becomes U+FFFD; a code point above
/// U+FFFF becomes a surrogate pair, never split across two calls.
std::size_t utf8_to_utf16(std::string_view text, std::size_t& at,
                          std::span<char16_t> out) noexcept;

/// Encode UTF-16 `units` as UTF-8 into `out`, whole code points only; returns
/// bytes written. An unpaired surrogate becomes U+FFFD.
std::size_t utf16_to_utf8(std::span<const char16_t> units, std::span<char> out) noexcept;

/// The keyboard, key characters and the clipboard, through JNI.
class AndroidText final {
public:
    AndroidText() noexcept = default;
    AndroidText(const AndroidText&) = delete;
    AndroidText& operator=(const AndroidText&) = delete;

    /// Look everything up. `env` must be attached to this thread, which is
    /// the only one that may use the object. False if any of it is missing;
    /// what was found still works.
    bool create(JNIEnv* env, ANativeActivity* activity) noexcept;

    /// Release the global references, on the same thread, before the
    /// environment is detached.
    void destroy() noexcept;

    /// Two routes each, because devices differ in which they honour: the
    /// NDK's, which NativeActivity runs on its UI thread for its own content
    /// view, then InputMethodManager on the decor view. Both are idempotent.
    void show_keyboard() noexcept;
    void hide_keyboard() noexcept;

    /// The character a key press types on its device with these meta keys:
    /// a code point, or 0 for none (a dead key, a function key, a control).
    char32_t character(std::int32_t device, std::int32_t key_code,
                       std::int32_t meta_state) noexcept;

    /// The clipboard, written in pieces: begin, append as often as needed,
    /// end. Built as a Java string a chunk at a time, through no buffer of
    /// the whole text.
    bool clip_begin() noexcept;
    void clip_append(std::string_view utf8) noexcept;
    bool clip_end() noexcept;

    /// The clipboard's text as UTF-8, as much as fits in `out`, cut at a
    /// character; the bytes written (0 when it is empty or not text).
    std::size_t clip_text(std::span<char> out) noexcept;

private:
    bool failed() noexcept;

    JNIEnv* env_{nullptr};
    ANativeActivity* native_{nullptr};
    jobject activity_{nullptr};
    // Global references, made in create().
    jclass key_map_class_{nullptr};
    jclass clip_data_class_{nullptr};
    jclass builder_class_{nullptr};
    jobject input_methods_{nullptr};
    jobject decor_view_{nullptr};
    jobject clipboard_{nullptr};
    // Method ids: valid as long as their classes are loaded, which system
    // classes always are.
    jmethodID key_map_load_{nullptr};
    jmethodID key_map_get_{nullptr};
    jmethodID show_soft_input_{nullptr};
    jmethodID hide_soft_input_{nullptr};
    jmethodID window_token_{nullptr};
    jmethodID builder_new_{nullptr};
    jmethodID builder_append_{nullptr};
    jmethodID to_string_{nullptr};
    jmethodID new_plain_text_{nullptr};
    jmethodID set_primary_clip_{nullptr};
    jmethodID get_primary_clip_{nullptr};
    jmethodID item_count_{nullptr};
    jmethodID item_at_{nullptr};
    jmethodID coerce_to_text_{nullptr};
    // The clip being built between clip_begin() and clip_end(): a local reference.
    jobject builder_{nullptr};
};

/// sq::app::TextInputService on Android: the keyboard shows while a widget
/// wants text. The entry point turns key presses into TextInput events while
/// active() is true.
class TextInput final : public sq::app::TextInputService {
public:
    explicit TextInput(AndroidText& text) noexcept : text_(text) {}

    void start(const sq::app::TextInputRequest&) override {
        active_ = true;
        text_.show_keyboard();
    }
    void update_area(const sq::app::TextInputArea&) override {}
    void stop() override {
        active_ = false;
        text_.hide_keyboard();
    }
    [[nodiscard]] bool active() const noexcept override { return active_; }

private:
    AndroidText& text_;
    bool active_{false};
};

}  // namespace sq_android
