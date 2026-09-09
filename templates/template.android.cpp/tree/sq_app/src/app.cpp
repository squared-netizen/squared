// {{project_name}} — application implementation.
//
// This file is YOURS (ownership class: seeded). Start here.
//
// What the generator gave you:
//
//   sq::gl    from kit.opengl, if you selected it — EGL context management and
//             a thin GLES 3.0 surface. Guarded with __has_include, so this file
//             compiles whether or not the kit was applied.
//
// The guard is not defensive clutter: it is what lets you add or swap a
// rendering kit later without editing code you have already written.

#include "app.hpp"

#if __has_include(<opengl/gl.hpp>)
#  include <opengl/gl.hpp>
#  define SQ_HAS_RENDERER 1
#endif

#include <android/log.h>

#include <cmath>

namespace {{project_name}} {
namespace {

constexpr const char* kTag = "{{project_name}}";

#define SQ_LOGI(...) __android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__)

}  // namespace

/// Private state, so app.hpp stays free of implementation detail and adding a
/// member does not rebuild everything that includes it.
struct App::State {
    int   width{0};
    int   height{0};
    float phase{0.0f};

    // Touch position in normalised coordinates, so the clear colour responds
    // to where you press. Proof the input path works, and the first thing to
    // delete.
    float touch_x{0.5f};
    float touch_y{0.5f};
    bool  pressed{false};
};

App::App() : state_(new State{}) {}

App::~App() { delete state_; }

void App::start() {
    SQ_LOGI("app start");
}

void App::stop() {
    SQ_LOGI("app stop");
}

void App::resume() {
    SQ_LOGI("app resume");
}

void App::pause() {
    SQ_LOGI("app pause");
}

void App::resize(int width, int height) {
    state_->width  = width;
    state_->height = height;
    SQ_LOGI("resize %dx%d", width, height);
#if defined(SQ_HAS_RENDERER)
    sq::gl::viewport(width, height);
#endif
}

void App::render() {
#if defined(SQ_HAS_RENDERER)
    // A slow pulse, so a still screenshot is not the only evidence the frame
    // loop is running. Delete this and draw something.
    state_->phase += 0.01f;
    const float pulse = 0.5f + 0.5f * std::sin(state_->phase);

    const sq::gl::Color colour{
        state_->pressed ? state_->touch_x : 0.05f,
        state_->pressed ? state_->touch_y : 0.25f + 0.25f * pulse,
        state_->pressed ? pulse : 0.12f,
        1.0f};

    sq::gl::clear(colour);
#endif
}

bool App::touch(TouchPhase phase, float x, float y) {
    if (state_->width > 0 && state_->height > 0) {
        state_->touch_x = x / static_cast<float>(state_->width);
        state_->touch_y = y / static_cast<float>(state_->height);
    }
    state_->pressed = (phase != TouchPhase::ended);

    if (phase == TouchPhase::began) {
        SQ_LOGI("touch at %.0f, %.0f", static_cast<double>(x), static_cast<double>(y));
    }
    return true;
}

}  // namespace {{project_name}}
