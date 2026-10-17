// {{project_name}} — the Java side of text. See android_text.hpp.
//
// This file is YOURS (ownership class: seeded), like entry.cpp.

#include "android_text.hpp"

#include <algorithm>
#include <array>

namespace sq_android {

namespace {

constexpr char32_t kReplacement = 0xFFFD;

/// KeyCharacterMap.COMBINING_ACCENT: the key starts an accent, typing nothing yet.
constexpr std::uint32_t kCombiningAccent = 0x80000000U;

}  // namespace

std::size_t encode_utf8(char32_t c, std::span<char, 4> out) noexcept {
    if (c < 0x80) {
        out[0] = static_cast<char>(c);
        return 1;
    }
    if (c < 0x800) {
        out[0] = static_cast<char>(0xC0 | (c >> 6));
        out[1] = static_cast<char>(0x80 | (c & 0x3F));
        return 2;
    }
    if (c >= 0xD800 && c <= 0xDFFF) return 0;
    if (c < 0x10000) {
        out[0] = static_cast<char>(0xE0 | (c >> 12));
        out[1] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (c & 0x3F));
        return 3;
    }
    if (c < 0x110000) {
        out[0] = static_cast<char>(0xF0 | (c >> 18));
        out[1] = static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        out[2] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out[3] = static_cast<char>(0x80 | (c & 0x3F));
        return 4;
    }
    return 0;
}

namespace {

/// One code point from `text` at `at`, moving `at` past it. Overlong forms,
/// surrogates, values past U+10FFFF and cut sequences give U+FFFD, consuming
/// one byte, so the next valid character still arrives.
char32_t decode_utf8(std::string_view text, std::size_t& at) noexcept {
    const auto byte = [&](std::size_t i) { return static_cast<unsigned char>(text[i]); };
    const unsigned char lead = byte(at);
    std::size_t length = 0;
    char32_t c = 0;
    char32_t least = 0;
    if (lead < 0x80) {
        ++at;
        return lead;
    } else if ((lead & 0xE0) == 0xC0) {
        length = 2; c = lead & 0x1F; least = 0x80;
    } else if ((lead & 0xF0) == 0xE0) {
        length = 3; c = lead & 0x0F; least = 0x800;
    } else if ((lead & 0xF8) == 0xF0) {
        length = 4; c = lead & 0x07; least = 0x10000;
    } else {
        ++at;
        return kReplacement;
    }
    if (at + length > text.size()) {
        ++at;
        return kReplacement;
    }
    for (std::size_t i = 1; i < length; ++i) {
        if ((byte(at + i) & 0xC0) != 0x80) {
            ++at;
            return kReplacement;
        }
        c = (c << 6) | (byte(at + i) & 0x3F);
    }
    if (c < least || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) {
        ++at;
        return kReplacement;
    }
    at += length;
    return c;
}

}  // namespace

std::size_t utf8_to_utf16(std::string_view text, std::size_t& at,
                          std::span<char16_t> out) noexcept {
    std::size_t written = 0;
    while (at < text.size()) {
        std::size_t next = at;
        const char32_t c = decode_utf8(text, next);
        const std::size_t units = c > 0xFFFF ? 2 : 1;
        if (written + units > out.size()) break;
        if (units == 2) {
            const char32_t v = c - 0x10000;
            out[written++] = static_cast<char16_t>(0xD800 + (v >> 10));
            out[written++] = static_cast<char16_t>(0xDC00 + (v & 0x3FF));
        } else {
            out[written++] = static_cast<char16_t>(c);
        }
        at = next;
    }
    return written;
}

std::size_t utf16_to_utf8(std::span<const char16_t> units, std::span<char> out) noexcept {
    std::size_t written = 0;
    for (std::size_t i = 0; i < units.size(); ++i) {
        char32_t c = units[i];
        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < units.size() && units[i + 1] >= 0xDC00 &&
            units[i + 1] <= 0xDFFF) {
            c = 0x10000 + ((c - 0xD800) << 10) + (units[i + 1] - 0xDC00);
            ++i;
        } else if (c >= 0xD800 && c <= 0xDFFF) {
            c = kReplacement;
        }
        std::array<char, 4> bytes{};
        const std::size_t length = encode_utf8(c, bytes);
        if (written + length > out.size()) break;
        for (std::size_t b = 0; b < length; ++b) out[written + b] = bytes[b];
        written += length;
    }
    return written;
}

bool AndroidText::failed() noexcept {
    if (env_->ExceptionCheck() == JNI_FALSE) return false;
    env_->ExceptionClear();
    return true;
}

bool AndroidText::create(JNIEnv* env, ANativeActivity* activity) noexcept {
    env_ = env;
    native_ = activity;
    activity_ = activity != nullptr ? activity->clazz : nullptr;
    if (env_ == nullptr || activity_ == nullptr) return false;
    bool complete = true;

    // A class by name, held globally; null and noted when missing.
    const auto global_class = [&](const char* name) -> jclass {
        jclass local = env_->FindClass(name);
        if (failed() || local == nullptr) {
            complete = false;
            return nullptr;
        }
        auto* global = static_cast<jclass>(env_->NewGlobalRef(local));
        env_->DeleteLocalRef(local);
        return global;
    };
    const auto method = [&](jclass type, const char* name, const char* signature) {
        jmethodID id = type != nullptr ? env_->GetMethodID(type, name, signature) : nullptr;
        const bool missing = failed() || id == nullptr;
        if (missing) complete = false;
        return missing ? nullptr : id;
    };
    const auto static_method = [&](jclass type, const char* name, const char* signature) {
        jmethodID id = type != nullptr ? env_->GetStaticMethodID(type, name, signature) : nullptr;
        const bool missing = failed() || id == nullptr;
        if (missing) complete = false;
        return missing ? nullptr : id;
    };

    key_map_class_ = global_class("android/view/KeyCharacterMap");
    key_map_load_ = static_method(key_map_class_, "load", "(I)Landroid/view/KeyCharacterMap;");
    key_map_get_ = method(key_map_class_, "get", "(II)I");

    builder_class_ = global_class("java/lang/StringBuilder");
    builder_new_ = method(builder_class_, "<init>", "()V");
    builder_append_ =
        method(builder_class_, "append", "(Ljava/lang/String;)Ljava/lang/StringBuilder;");
    jclass object_class = env_->FindClass("java/lang/Object");
    to_string_ = method(object_class, "toString", "()Ljava/lang/String;");
    if (object_class != nullptr) env_->DeleteLocalRef(object_class);

    clip_data_class_ = global_class("android/content/ClipData");
    new_plain_text_ = static_method(
        clip_data_class_, "newPlainText",
        "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Landroid/content/ClipData;");
    item_count_ = method(clip_data_class_, "getItemCount", "()I");
    item_at_ = method(clip_data_class_, "getItemAt", "(I)Landroid/content/ClipData$Item;");
    jclass item_class = env_->FindClass("android/content/ClipData$Item");
    coerce_to_text_ = method(item_class, "coerceToText",
                             "(Landroid/content/Context;)Ljava/lang/CharSequence;");
    if (item_class != nullptr) env_->DeleteLocalRef(item_class);
    failed();

    // The two system services, and the window's decor view for the keyboard.
    jclass activity_class = env_->GetObjectClass(activity_);
    jmethodID service = method(activity_class, "getSystemService",
                               "(Ljava/lang/String;)Ljava/lang/Object;");
    jmethodID window = method(activity_class, "getWindow", "()Landroid/view/Window;");
    const auto system_service = [&](const char* name) -> jobject {
        if (service == nullptr) return nullptr;
        jstring key = env_->NewStringUTF(name);
        jobject local = env_->CallObjectMethod(activity_, service, key);
        env_->DeleteLocalRef(key);
        if (failed() || local == nullptr) {
            complete = false;
            return nullptr;
        }
        jobject global = env_->NewGlobalRef(local);
        env_->DeleteLocalRef(local);
        return global;
    };
    input_methods_ = system_service("input_method");
    clipboard_ = system_service("clipboard");
    if (input_methods_ != nullptr) {
        jclass imm_class = env_->GetObjectClass(input_methods_);
        show_soft_input_ = method(imm_class, "showSoftInput", "(Landroid/view/View;I)Z");
        hide_soft_input_ = method(imm_class, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z");
        env_->DeleteLocalRef(imm_class);
    }
    if (clipboard_ != nullptr) {
        jclass clipboard_class = env_->GetObjectClass(clipboard_);
        set_primary_clip_ = method(clipboard_class, "setPrimaryClip", "(Landroid/content/ClipData;)V");
        get_primary_clip_ = method(clipboard_class, "getPrimaryClip", "()Landroid/content/ClipData;");
        env_->DeleteLocalRef(clipboard_class);
    }
    if (window != nullptr) {
        jobject window_object = env_->CallObjectMethod(activity_, window);
        if (!failed() && window_object != nullptr) {
            jclass window_class = env_->GetObjectClass(window_object);
            jmethodID decor = method(window_class, "getDecorView", "()Landroid/view/View;");
            jobject view = decor != nullptr ? env_->CallObjectMethod(window_object, decor) : nullptr;
            if (!failed() && view != nullptr) {
                decor_view_ = env_->NewGlobalRef(view);
                jclass view_class = env_->GetObjectClass(view);
                window_token_ = method(view_class, "getWindowToken", "()Landroid/os/IBinder;");
                env_->DeleteLocalRef(view_class);
                env_->DeleteLocalRef(view);
            } else {
                complete = false;
            }
            env_->DeleteLocalRef(window_class);
            env_->DeleteLocalRef(window_object);
        } else {
            complete = false;
        }
    }
    env_->DeleteLocalRef(activity_class);
    return complete;
}

void AndroidText::destroy() noexcept {
    if (env_ == nullptr) return;
    const auto release = [&](auto& global) {
        if (global != nullptr) env_->DeleteGlobalRef(global);
        global = nullptr;
    };
    release(key_map_class_);
    release(clip_data_class_);
    release(builder_class_);
    release(input_methods_);
    release(decor_view_);
    release(clipboard_);
    env_ = nullptr;
}

void AndroidText::show_keyboard() noexcept {
    if (native_ != nullptr) ANativeActivity_showSoftInput(native_, ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED);
    if (input_methods_ == nullptr || show_soft_input_ == nullptr || decor_view_ == nullptr) return;
    env_->CallBooleanMethod(input_methods_, show_soft_input_, decor_view_, 0);
    failed();
}

void AndroidText::hide_keyboard() noexcept {
    if (native_ != nullptr) ANativeActivity_hideSoftInput(native_, 0);
    if (input_methods_ == nullptr || hide_soft_input_ == nullptr || window_token_ == nullptr) return;
    jobject token = env_->CallObjectMethod(decor_view_, window_token_);
    if (!failed() && token != nullptr) {
        env_->CallBooleanMethod(input_methods_, hide_soft_input_, token, 0);
        failed();
        env_->DeleteLocalRef(token);
    }
}

char32_t AndroidText::character(std::int32_t device, std::int32_t key_code,
                                std::int32_t meta_state) noexcept {
    if (key_map_class_ == nullptr || key_map_load_ == nullptr || key_map_get_ == nullptr) return 0;
    jobject map = env_->CallStaticObjectMethod(key_map_class_, key_map_load_, device);
    if (failed() || map == nullptr) return 0;
    const jint value = env_->CallIntMethod(map, key_map_get_, key_code, meta_state);
    env_->DeleteLocalRef(map);
    if (failed()) return 0;
    const auto bits = static_cast<std::uint32_t>(value);
    return (bits & kCombiningAccent) != 0 ? 0 : static_cast<char32_t>(bits);
}

bool AndroidText::clip_begin() noexcept {
    if (clipboard_ == nullptr || set_primary_clip_ == nullptr || new_plain_text_ == nullptr ||
        builder_new_ == nullptr || builder_append_ == nullptr || to_string_ == nullptr) {
        return false;
    }
    builder_ = env_->NewObject(builder_class_, builder_new_);
    return !failed() && builder_ != nullptr;
}

void AndroidText::clip_append(std::string_view utf8) noexcept {
    if (builder_ == nullptr) return;
    // A chunk at a time: 512 UTF-16 units on the stack, one Java string each,
    // released before the next so the local reference table never fills.
    std::array<char16_t, 512> units{};
    std::size_t at = 0;
    while (at < utf8.size()) {
        const std::size_t count = utf8_to_utf16(utf8, at, units);
        if (count == 0) break;
        jstring piece = env_->NewString(reinterpret_cast<const jchar*>(units.data()),
                                        static_cast<jsize>(count));
        if (failed() || piece == nullptr) return;
        jobject self = env_->CallObjectMethod(builder_, builder_append_, piece);
        env_->DeleteLocalRef(piece);
        if (failed()) return;
        if (self != nullptr) env_->DeleteLocalRef(self);
    }
}

bool AndroidText::clip_end() noexcept {
    if (builder_ == nullptr) return false;
    bool done = false;
    jobject text = env_->CallObjectMethod(builder_, to_string_);
    if (!failed() && text != nullptr) {
        jstring label = env_->NewStringUTF("squared console");
        jobject clip = env_->CallStaticObjectMethod(clip_data_class_, new_plain_text_, label, text);
        if (!failed() && clip != nullptr) {
            env_->CallVoidMethod(clipboard_, set_primary_clip_, clip);
            done = !failed();
            env_->DeleteLocalRef(clip);
        }
        env_->DeleteLocalRef(label);
        env_->DeleteLocalRef(text);
    }
    env_->DeleteLocalRef(builder_);
    builder_ = nullptr;
    return done;
}

std::size_t AndroidText::clip_text(std::span<char> out) noexcept {
    if (clipboard_ == nullptr || get_primary_clip_ == nullptr || item_count_ == nullptr ||
        item_at_ == nullptr || coerce_to_text_ == nullptr || to_string_ == nullptr) {
        return 0;
    }
    std::size_t written = 0;
    jobject clip = env_->CallObjectMethod(clipboard_, get_primary_clip_);
    if (failed() || clip == nullptr) return 0;
    const jint items = env_->CallIntMethod(clip, item_count_);
    if (!failed() && items > 0) {
        jobject item = env_->CallObjectMethod(clip, item_at_, 0);
        if (!failed() && item != nullptr) {
            jobject sequence = env_->CallObjectMethod(item, coerce_to_text_, activity_);
            if (!failed() && sequence != nullptr) {
                auto text = static_cast<jstring>(env_->CallObjectMethod(sequence, to_string_));
                if (!failed() && text != nullptr) {
                    // Only what can fit: at most one UTF-16 unit per output byte.
                    std::array<char16_t, 512> units{};
                    const jsize length = env_->GetStringLength(text);
                    const auto take = static_cast<jsize>(
                        std::min<std::size_t>({static_cast<std::size_t>(length), units.size(),
                                               out.size()}));
                    env_->GetStringRegion(text, 0, take, reinterpret_cast<jchar*>(units.data()));
                    if (!failed()) {
                        written = utf16_to_utf8(
                            std::span<const char16_t>(units.data(), static_cast<std::size_t>(take)),
                            out);
                    }
                    env_->DeleteLocalRef(text);
                }
                env_->DeleteLocalRef(sequence);
            }
            env_->DeleteLocalRef(item);
        }
    }
    failed();
    env_->DeleteLocalRef(clip);
    return written;
}

}  // namespace sq_android
