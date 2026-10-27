#include <squared/app/event.hpp>
#include <squared/app/runtime.hpp>
#include <squared/files/file_handle.hpp>
#include <squared/files/file_system.hpp>
#include <squared/files/posix_file_system.hpp>
#include <squared/files/src/file_error.hpp>
#include <squared/files/src/file_error_code.hpp>
#include <squared/files/src/file_read_result.hpp>
#include <squared/files/src/file_type.hpp>
#include <squared/stage/app.hpp>
#include <squared/time/now.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace sq
{

namespace
{

/// The App's own directory in the app's private files, and what it keeps there.
constexpr std::string_view save_directory = "squared";
constexpr std::string_view model_file_name = "squared/model.state";
/// Where 7.6 and 7.7 kept the screens; removed at start, if one is left.
constexpr std::string_view old_screens_file = "squared/screens";

/// The first line of what the App gives the platform to keep: there even
/// with no screens to keep, so a kept activity is told from a new launch.
constexpr std::string_view state_header = "squared app 1\n";

/// The platform's paths, written and read with no roots: the activity's file
/// system may be gone (a save from the model's thread, after it).
[[nodiscard]] files::PosixFileSystem paths_only()
{
    return files::PosixFileSystem( files::PosixFileSystemRoots{} );
}

} // namespace

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
    const std::span<const std::byte> saved = saved_;
    saved_ = {};
    // A run held, and a new launch (Android kept nothing for this activity:
    // a swipe from the recent apps, with the process left alive): it starts
    // fresh, as after a quit. The model saved when the app left the screen.
    if( started_ && saved.empty() ) {
        shut_down();
    }
    stage.attach( runtime );
    if( !started_ ) {
        // The first window of the process, or the first since a quit.
        started_ = true;
        quit_ = false;
        visible_ = true;
        to_background_ = false;
        find_saves( runtime.files );
        squared_main( *this );
        // Every C++ screen names.json lists should be added by now.
        stage.check_screens();
        const std::string_view text( reinterpret_cast<const char *>( saved.data() ), saved.size() );
        restore_screens( text );
    }
    // A skin that will not load is not a reason to refuse to start: the
    // Stage draws dark red and has said why.
    return true;
}

void App::set_saved_state( std::span<const std::byte> saved ) noexcept
{
    saved_ = saved;
}

std::size_t App::save_state( std::span<std::byte> out ) noexcept
{
    if( quit_requested() || out.size() < state_header.size() ) {
        return 0; // a quit starts the next launch fresh
    }
    auto *text = reinterpret_cast<char *>( out.data() );
    std::copy( state_header.begin(), state_header.end(), text );
    std::size_t length = state_header.size();
    if( stage.keeps_screens() ) {
        // Nothing to show again, or the Stage said why not: the header alone.
        length += stage.save( { text + length, out.size() - length } );
    }
    return length;
}

void App::find_saves( files::FileSystem &files )
{
    model_path_.clear();
    if( const files::FileError error = files.local( save_directory ).make_directories() ) {
        // No private files at all (a host test) is not worth a line; a failure is.
        if( error.code != files::FileErrorCode::NotSupported ) {
            report_( "app: nothing will be kept: " + error.message + ": " + error.path );
        }
        return;
    }
    model_path_ = files.resolve_platform_path( files::FileType::Local, model_file_name );
    static_cast<void>( files.remove( files::FileType::Local, old_screens_file ) );
}

detail::ModelFile App::model_file()
{
    detail::ModelFile file;
    file.failed = &save_failed_;
    if( model_path_.empty() ) {
        return file;
    }
    file.path = model_path_;
    files::FileReadResult read = paths_only().read( files::FileType::Absolute, model_path_ );
    if( read ) {
        file.saved = std::move( read.bytes );
    } else if( read.error.code != files::FileErrorCode::NotFound ) {
        report_( "app: the model's last save could not be read: " + read.error.message );
    }
    return file;
}

void App::restore_screens( std::string_view saved )
{
    if( saved.empty() ) {
        return; // a new launch
    }
    if( !saved.starts_with( state_header ) ) {
        report_( "app: what Android kept is not this app's: the screens start fresh" );
        return;
    }
    const std::string_view screens = saved.substr( state_header.size() );
    if( screens.empty() || !stage.keeps_screens() ) {
        return;
    }
    static_cast<void>( stage.restore( screens ) );
}

void App::handle_event( const app::Event &event )
{
    using Type = app::Event::Type;
    if( stage.event( event ) ) {
        return;
    }
    // Nothing wanted it. Back at the root sends the app to the background,
    // as Home does (7.8: it quit); the platform asking to quit quits.
    if( event.type == Type::BackRequested ) {
        to_background_ = true;
    } else if( event.type == Type::QuitRequested ) {
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
    if( save_failed_.exchange( false, std::memory_order_relaxed ) ) {
        report_( "app: the model's save could not be written; the one before it stays" );
    }
}

void App::background()
{
    if( !visible_ ) {
        return;
    }
    visible_ = false;
    left_at_ = time::since_boot();
    line_.ui().background();
    // A model on this thread takes its turn now, so it saves before the app
    // stops: it may not run again. On its own thread it wakes for it.
    if( model_ != nullptr ) {
        static_cast<void>( model_->turn() );
    }
}

void App::low_memory()
{
    // Screens beneath the top keep no widgets already; the Stage lets go of
    // what it keeps warm, and the model hears of it.
    stage.low_memory();
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
    // What the platform handed over lives as long as its activity: the next
    // one hands its own (reopened from the recent apps, the process lives on).
    stage.set_text_input_service( nullptr );
    set_report( {} );
}

bool App::quit_requested() const noexcept
{
    return quit_ || stage.leave_requested();
}

bool App::take_background_request() noexcept
{
    const bool asked = stage.take_background_request(); // a `back` tap at the root
    return std::exchange( to_background_, false ) || asked;
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
    to_background_ = false;
}

} // namespace sq
