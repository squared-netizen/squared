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

#include <android/native_activity.h>
#include <jni.h>

import sq.app;

import std;

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
    /// `label` is what Android shows of the clip where it shows one.
    bool clip_end(const char* label = "squared console") noexcept;

    /// The clipboard's text as UTF-8, as much as fits in `out` of its first
    /// 512 UTF-16 units, cut at a character; the bytes written (0 when it is
    /// empty or not text). The Console's: a line.
    std::size_t clip_text(std::span<char> out) noexcept;

    /// The clipboard's whole text as UTF-8 into `out`: whole characters, in
    /// order, until one does not fit; the bytes the whole text takes (0 when
    /// it is empty or not text). Read 512 units at a time, no allocation.
    std::size_t clip_read(std::span<char> out) noexcept;

private:
    bool failed() noexcept;
    /// The clip's first item as text: a local reference, or nullptr.
    jstring clip_string() noexcept;

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
///
/// The keyboard is shown once no finger is down, at the next frame(), not in
/// start(): a widget takes the focus as a finger goes down, and a keyboard
/// shown while that touch is still going is put away again by Android when it
/// ends (sqterm's terminal on the phone, 2026-11-04; the Console's keyboard,
/// shown as its three-finger tap ends, stayed).
///
/// Some keyboards put themselves away after Enter when there is no text view
/// behind them (CleverKeys does): entered() asks for it again a few frames
/// later, as the Console does for its own input row. Asking for a keyboard
/// already up does nothing.
class TextInput final : public sq::app::TextInputService {
public:
    explicit TextInput(AndroidText& text) noexcept : text_(text) {}

    void start(const sq::app::TextInputRequest&) override {
        active_ = true;
        show_due_ = true;
    }
    void update_area(const sq::app::TextInputArea&) override {}
    void stop() override {
        active_ = false;
        show_due_ = false;
        again_frames_ = 0;
        text_.hide_keyboard();
    }
    [[nodiscard]] bool active() const noexcept override { return active_; }

    /// Each frame, with whether a finger is on the screen: the keyboard
    /// start() asked for, once none is.
    void frame(bool touching) noexcept {
        if (again_frames_ > 0 && --again_frames_ == 0 && active_) show_due_ = true;
        if (show_due_ && !touching) {
            show_due_ = false;
            text_.show_keyboard();
        }
    }

    /// Enter was typed while a widget wants text: the keyboard asked for
    /// again in about 100 ms, in case it put itself away.
    void entered() noexcept {
        if (active_) again_frames_ = 6;
    }

private:
    AndroidText& text_;
    bool active_{false};
    bool show_due_{false};
    std::uint8_t again_frames_{0};
};

/// sq::app::ClipboardService on Android: the system clipboard's text, through
/// AndroidText, on the glue thread (the App's). The entry point hands it to
/// the App after create().
class Clipboard final : public sq::app::ClipboardService {
public:
    explicit Clipboard(AndroidText& text) noexcept : text_(text) {}

    [[nodiscard]] std::size_t text(std::span<char> out) override { return text_.clip_read(out); }
    bool set_text(std::string_view utf8) override {
        if (!text_.clip_begin()) return false;
        text_.clip_append(utf8);
        return text_.clip_end("{{project_name}}");
    }

private:
    AndroidText& text_;
};

}  // namespace sq_android
