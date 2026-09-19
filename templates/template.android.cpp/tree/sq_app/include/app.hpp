// {{project_name}} — application.
//
// This file is YOURS (ownership class: seeded). The generator wrote it once and
// will never overwrite it.
//
// Note what is NOT here: no Android header, no EGL, no GLES. This class is
// portable — the platform layer in sq_android/ calls into it, and the same
// class compiles under template.termux.cpp with a different platform layer.
// Keeping Android out of sq_app/ is what makes that true, and it is worth
// defending as you add code.
//
// It implements sq::app::Application, which is squared's contract rather than
// this template's. That means the platform layer, the GUI, and every squared
// subsystem already know how to talk to it.

#ifndef {{project_name}}_APP_HPP
#define {{project_name}}_APP_HPP

#include <squared/app/application.hpp>
#include <squared/graphics/color.hpp>

#include <chrono>
#include <memory>

namespace {{project_name}} {

/// The application.
///
/// Every method is called from the platform layer on the main thread. None of
/// them may block: Android will kill the process if the main thread stops
/// responding, and there is no warning first.
class App final : public sq::app::Application {
public:
    App();
    ~App() override;

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /// Once, before the first frame. Return false to abort startup.
    [[nodiscard]] bool create(sq::graphics::Context& graphics) override;

    /// Input, lifecycle and text. One event at a time, already translated out
    /// of whatever the platform speaks.
    void handle_event(const sq::app::Event& event) override;

    /// One simulation step. Separate from render() on purpose: this is where
    /// game logic goes, and it is the one that should not care how long the
    /// frame took to draw.
    void update(std::chrono::nanoseconds delta) override;

    /// One frame. Only called while there is a live rendering surface.
    void render(sq::graphics::Context& graphics) override;

    /// Focus lost. Save anything you cannot afford to lose — Android may
    /// destroy the process after this without calling dispose().
    void pause() override;

    /// The window is visible and focused again.
    void resume() override;

    /// The drawable size changed. Called before the first render, and again on
    /// rotation.
    void resize(int width, int height) override;

    /// A new rendering surface exists. On Android this happens more than once
    /// per process: every return from the background gets a fresh one, and GPU
    /// objects may or may not have survived. Ask the context which.
    void surface_created(sq::graphics::Context& graphics) override;

    /// The rendering surface is gone. Do not issue graphics calls after this
    /// until surface_created() runs again.
    void surface_destroyed() override;

    /// Once, on the way out. The rendering surface may already be gone.
    void dispose() override;

    /// Whether the application wants to exit.
    [[nodiscard]] bool quit_requested() const noexcept override;

private:
    struct State;
    std::unique_ptr<State> state_;
};

}  // namespace {{project_name}}

#endif  // {{project_name}}_APP_HPP
