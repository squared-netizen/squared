// sq_imgui.hpp — SFML bridge for Dear ImGui.
//
// This is THIS KIT's code, not upstream: ImGui owns the UI, SFML owns the
// window, input and GL context, and nothing upstream knows how to meet an
// sf::RenderTarget. imgui_impl_android was trimmed because SFML already owns
// input; the two halves meet here instead.
//
// Deliberately header-light: sf::RenderTarget, sf::Event and sf::Time are
// forward-declared rather than included, so a file that only hands events to
// the bridge pays for none of SFML. The full types are needed only in
// sq_imgui.cpp.
//
// The target is sf::RenderTarget&, not sf::RenderWindow&, for the same reason
// App::render takes sf::RenderTarget& (spec decision D-071): the GL backend
// needs the current context and a display size, not a handle, so the same
// drawing code works against an sf::RenderTexture as well as the window.
//
// One touch is one mouse (spec decision: ImGui treats a single touch as a
// mouse, which is sufficient for touch UI). Only the first finger is
// translated; multi-touch past finger 0 is ignored, matching the template's
// App::touch contract.

#ifndef IMGUI_SQ_IMGUI_HPP
#define IMGUI_SQ_IMGUI_HPP

namespace sf {
class RenderTarget;
class Event;
class Time;
}  // namespace sf

namespace sq {
namespace imgui {

/// Owns the ImGui context and the OpenGL3 backend for one process.
///
/// Call init() once, with the GL context current (the template guarantees
/// this: App::start() runs with a current GLES 3.0 context). Route every
/// sf::Event to processEvent(), write state under ImGui widgets, and render
/// once per frame. The GL context must still be current when render() runs,
/// which it is during App::render.
///
/// All methods are called from SFML's one thread, so the class needs no
/// synchronisation, exactly like the template's App.
class Bridge {
public:
    Bridge() = default;
    ~Bridge() = default;

    Bridge(const Bridge&)            = delete;
    Bridge& operator=(const Bridge&) = delete;

    /// Create the ImGui context and initialise the OpenGL3 backend.
    ///
    /// Target GLES 3.0: the backend is passed "#version 300 es". The backend
    /// itself detected ES3 on Android, so this string and its auto-detection
    /// agree.
    bool init();

    /// Tear down the backend and destroy the ImGui context.
    void shutdown();

    /// Translate one SFML event into ImGui input.
    ///
    /// TouchBegan/Moved/Ended (finger 0) become a mouse at the touch point;
    /// Resized updates io.DisplaySize. Everything else is ignored — SFML has
    /// already owned it.
    void processEvent(const sf::Event& event);

    /// Begin a frame. Call before any ImGui widgets for the frame.
    ///
    /// Sets DisplaySize from the target (the same target the API renders to,
    /// so an sf::RenderTexture sized differently from the window renders the
    /// UI at its own resolution) and DeltaTime from `dt`.
    void newFrame(const sf::Time& dt, sf::RenderTarget& target);

    /// End the frame and draw. The GL context must be current.
    void render();

private:
    bool initialized_ = false;
};

}  // namespace imgui
}  // namespace sq

#endif