// {{project_name}} — application interface.
//
// This file is YOURS (ownership class: seeded). The generator wrote it once and
// will never overwrite it.
//
// Note what is NOT here: no Android header, no EGL, no GLES. This class is
// portable — the platform layer in sq_android/ calls into it, and the same
// class compiles under template.termux.cpp with a different platform layer.
// Keeping Android out of sq_app/ is what makes that true, and it is worth
// defending as you add code.

#ifndef {{project_name}}_APP_HPP
#define {{project_name}}_APP_HPP

namespace {{project_name}} {

/// A touch, reduced to the three phases an application actually branches on.
enum class TouchPhase { began, moved, ended };

/// The application.
///
/// Every method is called from the platform layer on the main thread. None of
/// them may block: Android will kill the process if the main thread stops
/// responding, and there is no warning first.
class App {
public:
    App();
    ~App();

    App(const App&)            = delete;
    App& operator=(const App&) = delete;

    /// Once, before the first frame.
    void start();

    /// Once, on the way out. The rendering surface may already be gone.
    void stop();

    /// The window is visible and focused.
    void resume();

    /// Focus lost. Save anything you cannot afford to lose — Android may
    /// destroy the process after this without calling stop().
    void pause();

    /// The drawable size changed. Called before the first render, and again on
    /// rotation.
    void resize(int width, int height);

    /// One frame. Only called while there is a live rendering surface.
    void render();

    /// A touch. Return true if you handled it.
    bool touch(TouchPhase phase, float x, float y);

private:
    struct State;
    State* state_;
};

}  // namespace {{project_name}}

#endif  // {{project_name}}_APP_HPP
