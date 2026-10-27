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
constexpr std::string_view screens_file = "squared/screens";
constexpr std::string_view model_file_name = "squared/model.state";

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
    stage.attach( runtime );
    if( !started_ ) {
        // The first window of the process, or the first since a quit.
        started_ = true;
        quit_ = false;
        visible_ = true;
        find_saves( runtime.files );
        squared_main( *this );
        // Every C++ screen names.json lists should be added by now.
        stage.check_screens();
        restore_screens();
    }
    // A skin that will not load is not a reason to refuse to start: the
    // Stage draws dark red and has said why.
    return true;
}

void App::find_saves( files::FileSystem &files )
{
    screens_path_.clear();
    model_path_.clear();
    if( const files::FileError error = files.local( save_directory ).make_directories() ) {
        // No private files at all (a host test) is not worth a line; a failure is.
        if( error.code != files::FileErrorCode::NotSupported ) {
            report_( "app: nothing will be kept: " + error.message + ": " + error.path );
        }
        return;
    }
    screens_path_ = files.resolve_platform_path( files::FileType::Local, screens_file );
    model_path_ = files.resolve_platform_path( files::FileType::Local, model_file_name );
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

void App::restore_screens()
{
    if( screens_path_.empty() ) {
        return;
    }
    files::PosixFileSystem files = paths_only();
    const files::FileReadResult read = files.read( files::FileType::Absolute, screens_path_ );
    if( !read ) {
        return; // none: a first start, or the last run ended with a quit
    }
    // Shown once: a stack that cannot be built does not come back at every start.
    forget_screens();
    if( !stage.keeps_screens() ) {
        return;
    }
    const std::string_view text( reinterpret_cast<const char *>( read.bytes.data() ),
                                 read.bytes.size() );
    static_cast<void>( stage.restore( text ) );
}

void App::save_screens()
{
    if( screens_path_.empty() ) {
        return;
    }
    if( !stage.keeps_screens() || quit_requested() ) {
        forget_screens();
        return;
    }
    std::array<char, Stage::saved_bytes> text{};
    const std::size_t length = stage.save( text );
    if( length == 0 ) {
        forget_screens(); // nothing to show again, or the Stage said why not
        return;
    }
    const auto *bytes = reinterpret_cast<const std::byte *>( text.data() );
    const files::FileError error = paths_only().write_atomically(
        files::FileType::Absolute, screens_path_, std::span<const std::byte>( bytes, length ) );
    if( error ) {
        report_( "app: the screens were not saved: " + error.message );
    }
}

void App::forget_screens() noexcept
{
    if( !screens_path_.empty() ) {
        static_cast<void>( paths_only().remove( files::FileType::Absolute, screens_path_ ) );
    }
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
    save_screens();
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

void App::shut_down() noexcept
{
    model_.reset();
    // A line is made for one model: it keeps that model's quit, its values
    // and its rings. The next start gets a new one, in the same storage, so
    // the Stage's reference to its UI end stays good.
    std::destroy_at( &line_ );
    std::construct_at( &line_ );
    stage.clear();
    // A quit forgets where the user was: the next start begins at the start.
    forget_screens();
    started_ = false;
    quit_ = false;
}

} // namespace sq
