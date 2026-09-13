// {{project_name}} — application interface.
//
// This file is YOURS (ownership class: seeded). The generator wrote it once and
// will never overwrite it.
//
// ## This layer uses SFML, and that is deliberate
//
// An earlier revision of this template kept sq_app/ free of SFML so that the
// same class would compile unchanged under template.android.cpp. That rule has
// been withdrawn. It bought portability to a template that has no Graphics and
// no Audio, at the cost of making the modules this one exists to provide —
// sf::Texture, sf::Font, sf::Text, sf::Sound — unreachable from the only place
// application code is supposed to live.
//
// So: include what you need from <SFML/Graphics.hpp> and <SFML/Audio.hpp> in
// your .cpp files freely.
//
// ## What this layer still does not do
//
// It does not own the window, the event loop, the GL context or the activity.
// Those belong to sq_android/main.cpp. You are handed a render target and
// called back; you do not call back into the platform.
//
// Keeping <android/...> out of here is still worth doing. SFML already
// abstracts the platform, so reaching for Android headers directly means
// reaching past the thing that makes this portable between devices.
//
// ## Why the header forward-declares rather than includes
//
// sf::RenderTarget is forward-declared below, not included. <SFML/Graphics.hpp>
// is a large header, and every file that includes app.hpp would pay for it.
// Include the SFML headers you need in the .cpp instead.

#ifndef {{project_name}}_APP_HPP
#define {{project_name}}_APP_HPP

namespace sf {
class RenderTarget;
}

namespace {{project_name}} {

/// A touch, reduced to the three phases an application actually branches on.
enum class TouchPhase { began, moved, ended };

/// The application.
///
/// Every method is called from the platform layer, on one thread, so this class
/// needs no synchronisation of its own.
///
/// That thread is SFML's, not Android's UI thread: SFML receives the lifecycle
/// callbacks on the UI thread and runs the platform layer on a thread it owns.
/// Blocking here will not trigger an ANR the way it would on the UI thread —
/// but it still stops drawing and stops draining events, so do not block.
class App {
public:
    App();
    ~App();

    App(const App&)            = delete;
    App& operator=(const App&) = delete;

    /// Once, before the first frame. The GL context is already current.
    void start();

    /// Once, at shutdown.
    void stop();

    /// Returning to the foreground.
    void resume();

    /// Going to the background.
    ///
    /// Android may destroy the process after this without calling stop(). Save
    /// anything you cannot afford to lose here, not in stop().
    void pause();

    /// At startup and on every size change, including rotation.
    void resize(int width, int height);

    /// One frame. Draw into `target`; the platform layer presents it.
    ///
    /// `target` is the window, as an sf::RenderTarget. Taking the base class
    /// rather than sf::RenderWindow means the same drawing code works against
    /// an sf::RenderTexture — which is how you would screenshot, or render at
    /// a different resolution than you present at.
    void render(sf::RenderTarget& target);

    /// A touch. Return true if it was consumed.
    ///
    /// Only the first finger is forwarded. Multi-touch differs enough between
    /// platforms that it belongs in sq_android/main.cpp; widen this signature
    /// there if you need it.
    bool touch(TouchPhase phase, float x, float y);

private:
    struct State;
    State* state_;
};

}  // namespace {{project_name}}

#endif
