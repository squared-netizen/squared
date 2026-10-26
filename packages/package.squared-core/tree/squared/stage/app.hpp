#pragma once

#include <squared/app/application.hpp>
#include <squared/messaging/model_runner.hpp>
#include <squared/messaging/telegraph.hpp>
#include <squared/stage/stage.hpp>
#include <squared/time/time_point.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>

namespace sq
{

namespace detail
{

/** @brief A model's runner, whatever the model's type: what `App::run` keeps. */
class ModelHolder
{
    public:
        ModelHolder() noexcept = default;
        virtual ~ModelHolder() = default;
        ModelHolder( const ModelHolder & ) = delete;
        ModelHolder &operator=( const ModelHolder & ) = delete;
        ModelHolder( ModelHolder && ) = delete;
        ModelHolder &operator=( ModelHolder && ) = delete;

        /** @brief A model on the UI thread: one turn. On its own thread: nothing. */
        virtual std::uint32_t turn() noexcept = 0;
};

template<typename M>
class ModelOf final : public ModelHolder
{
    public:
        template<typename... Args>
        explicit ModelOf( messaging::TelegraphLine &line, Args &&...args ) noexcept
            : runner_( line, std::forward<Args>( args )... )
        {}

        std::uint32_t turn() noexcept override { return runner_.threaded() ? 0 : runner_.turn(); }

    private:
        messaging::ModelRunner<M> runner_;
};

} // namespace detail

/**
 * @brief The app: the Telegraph, the model at its far end, and the Stage at
 * its near one. One for the whole process.
 *
 * The platform makes it once and keeps it while the process lives, so an
 * activity Android recreates reattaches to the same App and the model keeps
 * running. It calls `squared_main` once, when the first window is there and
 * the skin is loaded; the app pushes its first screen there and starts its
 * model with `run`. Quitting (`quit()`, a `quit` tap, back at the root)
 * stops the model, after it saves, and empties the stack: the next launch in
 * the same process starts afresh.
 *
 * It is the platform's `sq::app::Application`: an app does not call those
 * methods, the platform does.
 */
class App final : public app::Application
{
        // First, so it is made before the Stage, which holds its UI end, and
        // destroyed after the model, which holds the other.
        messaging::TelegraphLine line_;

    public:
        /** @brief The screens: `app.stage.push( layouts::landing )`. */
        Stage stage;

        App() noexcept;
        ~App() override;
        App( const App & ) = delete;
        App &operator=( const App & ) = delete;
        App( App && ) = delete;
        App &operator=( App && ) = delete;

        /**
         * @brief Start the model `M` at the far end of the Telegraph, made
         * from `args` on its own thread (named "model"), or on this one with
         * `messaging::on_ui_thread` first, one turn a frame. One model to an
         * app: false, and reported, for a second.
         *
         * Allocates the runner, the model inside it, once.
         */
        template<typename M, typename... Args>
        bool run( Args &&...args )
        {
            if( model_ != nullptr ) {
                report_( "app: run: a model runs already; an app has one" );
                return false;
            }
            model_ = std::make_unique<detail::ModelOf<M>>( line_, std::forward<Args>( args )... );
            return true;
        }

        /** @brief Whether a model runs. */
        [[nodiscard]] bool running() const noexcept { return model_ != nullptr; }

        /** @brief Quit: the model saves and stops, and the platform closes the app. */
        void quit() noexcept { quit_ = true; }

        /** @brief The UI end of the Telegraph (the Stage's). */
        [[nodiscard]] messaging::TelegraphUi &telegraph() noexcept { return line_.ui(); }

        // sq::app::Application, for the platform.
        void set_text_input_service( app::TextInputService *service ) noexcept override;
        void set_report( app::Report report ) noexcept override;
        [[nodiscard]] bool create( app::Runtime &runtime ) override;
        void handle_event( const app::Event &event ) override;
        void update( std::chrono::nanoseconds delta ) override;
        void render( graphics::Context &graphics ) override;
        void foreground() override;
        void background() override;
        void low_memory() override;
        void resize( int width, int height ) override;
        void safe_area_changed( const app::SafeArea &area ) override;
        void surface_created( graphics::Context &graphics ) override;
        void dispose() override;
        [[nodiscard]] bool quit_requested() const noexcept override;

    private:
        void shut_down() noexcept;

        std::unique_ptr<detail::ModelHolder> model_;
        app::Report report_{};
        time::TimePoint left_at_{};
        bool started_ = false;
        bool quit_ = false;
        bool visible_ = true;
};

// The Telegraph line (88,000 bytes at the default knobs, which size it), then
// the Stage, the model's runner, and the App's own few bytes.
static_assert( sizeof( App ) == sizeof( messaging::TelegraphLine ) + 1216 );

} // namespace sq

/**
 * @brief The app's own start: written by the app, called once by the App when
 * the first window is up. Push the first screen, and start the model:
 *
 *     void squared_main( sq::App &app ) { app.run<Prefs>(); app.stage.push( layouts::landing ); }
 */
void squared_main( sq::App &app );
