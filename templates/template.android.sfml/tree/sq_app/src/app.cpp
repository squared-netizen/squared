// {{project_name}} — application implementation.
//
// This file is YOURS (ownership class: seeded). Start here.
//
// What the generator gave you: a live GLES 3.0 context, already current on this
// thread. SFML created it and the platform layer in sq_android/ presents it
// after every render() — so this file draws, and nothing else.
//
// Note what this file does NOT include: no SFML header. The platform layer owns
// the window, the event loop and the context; sq_app/ owns what is drawn into
// it. That separation is what lets the same App compile under
// template.android.cpp, which reaches the same GLES through NativeActivity and
// kit.opengl instead. Reaching for sf:: here would tie your application to one
// template, so if you find yourself wanting it, consider whether the thing you
// want belongs in sq_android/main.cpp instead.

#include "app.hpp"

#include <GLES3/gl3.h>

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
    bool  touching{false};
};

App::App() : state_(new State) {}

App::~App() {
    delete state_;
}

void App::start() {
    SQ_LOGI("start");
    SQ_LOGI("renderer: %s", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    SQ_LOGI("gles:     %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
}

void App::stop() {
    SQ_LOGI("stop");
}

void App::resume() {
    SQ_LOGI("resume");
}

void App::pause() {
    // Android may destroy the process after this without calling stop(). Save
    // anything you cannot afford to lose here, not in stop().
    SQ_LOGI("pause");
}

void App::resize(int width, int height) {
    state_->width  = width;
    state_->height = height;
    glViewport(0, 0, width, height);
    SQ_LOGI("resize %dx%d", width, height);
}

void App::render() {
    // A slow colour cycle, so that a freshly generated project visibly runs.
    // Replace it.
    state_->phase += 0.01f;

    const float wave = (std::sin(state_->phase) + 1.0f) * 0.5f;
    const float r    = state_->touching ? 0.85f : wave * 0.25f;
    const float g    = wave * 0.45f;
    const float b    = 0.35f + wave * 0.35f;

    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

bool App::touch(TouchPhase phase, float x, float y) {
    switch (phase) {
        case TouchPhase::began:
            state_->touching = true;
            SQ_LOGI("touch began %.0f,%.0f", static_cast<double>(x), static_cast<double>(y));
            return true;
        case TouchPhase::moved:
            return true;
        case TouchPhase::ended:
            state_->touching = false;
            return true;
    }
    return false;
}

}  // namespace {{project_name}}
