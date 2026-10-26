#include <squared/app/event.hpp>
#include <squared/stage/app.hpp>
#include <squared/time/now.hpp>

#include <chrono>
#include <cstdint>
#include <memory>

namespace sq
{

App::App() noexcept
    : stage( line_.ui() )
{}

App::~App()
{
    // The model first: it saves, and its thread is joined while the
    // Telegraph it uses still exists.
    model_.reset();
}

void App::set_text_input_service( app::TextInputService *service ) noexcept
{
    stage.set_text_input_service( service );
}

void App::set_report( app::Report report ) noexcept
{
    report_ = report;
    stage.set_report( report );
}

bool App::create( app::Runtime &runtime )
{
    stage.attach( runtime );
    if( !started_ ) {
        // The first window of the process, or the first since a quit.
        started_ = true;
        quit_ = false;
        visible_ = true;
        squared_main( *this );
        // Every C++ screen names.json lists should be added by now.
        stage.check_screens();
    }
    // A skin that will not load is not a reason to refuse to start: the
    // Stage draws dark red and has said why.
    return true;
}

void App::handle_event( const app::Event &event )
{
    using Type = app::Event::Type;
    if( stage.event( event ) ) {
        return;
    }
    // Nothing wanted it: back at the root, or the platform asking, leaves.
    if( event.type == Type::BackRequested || event.type == Type::QuitRequested ) {
        quit_ = true;
    }
}

void App::update( std::chrono::nanoseconds delta )
{
    if( model_ != nullptr ) {
        static_cast<void>( model_->turn() );
    }
    stage.update( delta );
}

void App::render( graphics::Context &graphics )
{
    stage.render( graphics );
}

void App::foreground()
{
    if( visible_ ) {
        return;
    }
    visible_ = true;
    const time::TimePoint away = time::since_boot() - left_at_;
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>( away );
    line_.ui().foreground( static_cast<std::uint64_t>( milliseconds.count() ) );
}

void App::background()
{
    if( !visible_ ) {
        return;
    }
    visible_ = false;
    left_at_ = time::since_boot();
    line_.ui().background();
}

void App::low_memory()
{
    // The Stage has nothing to drop: a screen beneath the top keeps no widgets.
    line_.ui().low_memory();
}

void App::resize( int width, int height )
{
    stage.resize( width, height );
}

void App::safe_area_changed( const app::SafeArea &area )
{
    stage.set_safe_area( area );
}

void App::surface_created( graphics::Context &graphics )
{
    stage.surface_created( graphics );
}

void App::dispose()
{
    // The activity is going. Quitting: the model saves and stops, and the
    // next launch starts afresh. Otherwise only the window's half goes; the
    // model runs on, and the next activity reattaches.
    if( quit_requested() ) {
        shut_down();
    }
    stage.detach();
}

bool App::quit_requested() const noexcept
{
    return quit_ || stage.leave_requested();
}

void App::shut_down() noexcept
{
    model_.reset();
    // A line is made for one model: it keeps that model's quit, its values
    // and its rings. The next start gets a new one, in the same storage, so
    // the Stage's reference to its UI end stays good.
    std::destroy_at( &line_ );
    std::construct_at( &line_ );
    stage.clear();
    started_ = false;
    quit_ = false;
}

} // namespace sq
