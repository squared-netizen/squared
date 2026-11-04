export module sq.app:src.background_service;

import std;

export namespace sq::app
{

/**
 * @brief Running while the app is away: what the platform does to keep the
 * process, and its model, alive with the app off the screen (Android: a
 * foreground service with a notification that says so).
 *
 * Implemented by the platform adapter; an application gets it through
 * Application::set_background_service, before create(), and keeps it for
 * the whole process (it outlives each activity). Where the platform has none,
 * or the app has not declared it (Android: dev.squared.ModelService in the
 * manifest), `available()` is false and the calls do nothing. Called on the
 * application thread.
 *
 * A person can end it from the notification (Exit): the application is then
 * sent `Event::Type::QuitRequested`. They can also hold the device awake with
 * the screen off there (Android's wake lock); that is the platform's own.
 */
class BackgroundService
{
    public:
        virtual ~BackgroundService() = default;

        /** @brief The app declared it and the platform can do it. */
        [[nodiscard]] virtual bool available() const noexcept = 0;

        /**
         * @brief Keep running, with a notification titled `title` saying
         * `text` (each at most 63 bytes of UTF-8; longer is cut at a
         * character). Again to change the text. False when not available.
         */
        virtual bool keep_running( std::string_view title, std::string_view text ) = 0;

        /** @brief Stop: the notification goes, and the wake lock if held. Idempotent. */
        virtual void stop_running() = 0;
};

} // namespace sq::app
