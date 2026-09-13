// {{project_name}} — application implementation.
//
// This file is YOURS (ownership class: seeded). Start here.
//
// What the generator gave you: an SFML render target, a live GLES 3.0 context,
// and the five modules kit.sfml ships — System, Window, Graphics, Audio and
// the activity entry point.
//
// The demo below draws without loading anything, so a freshly generated
// project runs before you have added a single asset.

#include "app.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <android/log.h>

#include <algorithm>
#include <cmath>
#include <optional>

namespace {{project_name}} {
namespace {

constexpr const char* kTag = "{{project_name}}";

#define SQ_LOGI(...) __android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__)
#define SQ_LOGW(...) __android_log_print(ANDROID_LOG_WARN, kTag, __VA_ARGS__)

// Assets live in sq_android/assets/ and are packaged into the APK by
// mk/squared_android_package.mk.
//
// Open them by bare name, relative to the assets root: sq_android/assets/
// font.ttf is "font.ttf", sq_android/assets/ui/panel.png is "ui/panel.png".
// SFML's Android FileInputStream hands the name unchanged to
// AAssetManager_open, which addresses into the APK's assets/ tree, so
// subdirectories survive and no prefix is wanted.
//
// Nothing is unpacked to storage: assets are read out of the APK in place.

}  // namespace

/// Private state, so app.hpp stays free of implementation detail and adding a
/// member does not rebuild everything that includes it.
struct App::State {
    sf::Vector2f size{0.f, 0.f};
    float        spin{0.f};
    bool         touching{false};
    sf::Vector2f touchPoint{0.f, 0.f};

    // Optional, because a freshly generated project has no assets yet and
    // must still run. Replace with whatever your project actually needs.
    std::optional<sf::Font> font;
};

App::App() : state_(new State) {}

App::~App() {
    delete state_;
}

void App::start() {
    SQ_LOGI("start");

    // Load a font if one was dropped into sq_android/assets/. Failure is not
    // fatal: the demo below degrades to shapes.
    sf::Font candidate;
    if (candidate.openFromFile("font.ttf")) {
        state_->font = std::move(candidate);
        SQ_LOGI("font.ttf loaded");
    } else {
        SQ_LOGW("no font.ttf in assets; drawing shapes only");
    }
}

void App::stop() {
    SQ_LOGI("stop");
}

void App::resume() {
    SQ_LOGI("resume");
}

void App::pause() {
    // Android may destroy the process after this without calling stop().
    SQ_LOGI("pause");
}

void App::resize(int width, int height) {
    state_->size = { static_cast<float>(width), static_cast<float>(height) };
    SQ_LOGI("resize %dx%d", width, height);
}

void App::render(sf::RenderTarget& target) {
    state_->spin += 0.6f;

    target.clear(sf::Color(18, 22, 34));

    // A rotating square, so that a freshly generated project visibly runs and
    // visibly animates. Replace it.
    const float side = std::min(state_->size.x, state_->size.y) * 0.28f;
    sf::RectangleShape box({ side, side });
    box.setOrigin({ side * 0.5f, side * 0.5f });
    box.setPosition({ state_->size.x * 0.5f, state_->size.y * 0.42f });
    box.setRotation(sf::degrees(state_->spin));
    box.setFillColor(state_->touching ? sf::Color(230, 90, 70)
                                      : sf::Color(80, 170, 220));
    target.draw(box);

    // Where you last touched, so input is visible without reading a log.
    if (state_->touching) {
        sf::CircleShape dot(18.f);
        dot.setOrigin({ 18.f, 18.f });
        dot.setPosition(state_->touchPoint);
        dot.setFillColor(sf::Color(250, 240, 120));
        target.draw(dot);
    }

    // Text only if a font was found. Drawing the rest unconditionally is
    // deliberate: a gate that hides the whole scene when one asset is missing
    // makes every later failure invisible too.
    if (state_->font) {
        sf::Text label(*state_->font, "{{project_name}}", 48);
        label.setFillColor(sf::Color::White);
        const sf::FloatRect bounds = label.getLocalBounds();
        label.setOrigin({ bounds.size.x * 0.5f, bounds.size.y * 0.5f });
        label.setPosition({ state_->size.x * 0.5f, state_->size.y * 0.78f });
        target.draw(label);
    }
}

bool App::touch(TouchPhase phase, float x, float y) {
    state_->touchPoint = { x, y };
    switch (phase) {
        case TouchPhase::began:
            state_->touching = true;
            SQ_LOGI("touch %.0f,%.0f", static_cast<double>(x), static_cast<double>(y));
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
