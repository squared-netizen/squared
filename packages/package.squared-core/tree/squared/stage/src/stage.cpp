#include "detail/catalog.hpp"

#include <squared/app/runtime.hpp>
#include <squared/files/file_handle.hpp>
#include <squared/files/file_system.hpp>
#include <squared/files/src/file_text_result.hpp>
#include <squared/graphics/atlas_region.hpp>
#include <squared/graphics/bitmap_font.hpp>
#include <squared/graphics/color.hpp>
#include <squared/graphics/context.hpp>
#include <squared/graphics/orthographic_camera.hpp>
#include <squared/graphics/sprite_batch.hpp>
#include <squared/graphics/src/bitmap_font_page.hpp>
#include <squared/graphics/texture_atlas.hpp>
#include <squared/graphics/texture_region.hpp>
#include <squared/gui/batch_painter.hpp>
#include <squared/gui/instantiate.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/skin_loader.hpp>
#include <squared/gui/src/font_ptr.hpp>
#include <squared/gui/src/font_resource.hpp>
#include <squared/gui/src/insets.hpp>
#include <squared/gui/src/skin_load_issue.hpp>
#include <squared/gui/src/skin_load_report.hpp>
#include <squared/gui/src/skin_load_severity.hpp>
#include <squared/gui/table.hpp>
#include <squared/gui/ui.hpp>
#include <squared/messaging/telegram_data.hpp>
#include <squared/messaging/telegram_text.hpp>
#include <squared/stage/stage.hpp>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sq
{

namespace
{

/// Where the default skin is, under the app's bundled files (7.4 moves it into the package).
constexpr std::string_view skin_directory = "skins/default/skin";

/// The logical length of the drawable's shorter side: a widget is the same
/// size on every screen, and one logical unit is square either way up.
constexpr float logical_short_side = 720.0F;

/// A failure (the skin did not load, a layout did not build), and the background.
constexpr graphics::Color dark_red{ .red = 0.55F, .green = 0.05F, .blue = 0.05F, .alpha = 1.0F };
constexpr graphics::Color slate{ .red = 0.25F, .green = 0.25F, .blue = 0.28F, .alpha = 1.0F };

[[nodiscard]] bool is_pointer( app::Event::Type type ) noexcept
{
    using Type = app::Event::Type;
    return type == Type::PointerDown || type == Type::PointerMove || type == Type::PointerUp;
}

/// Built short: a custom type or a drawable missing. The rest still shows.
[[nodiscard]] bool builds_short( gui::LayoutProblem problem ) noexcept
{
    using Problem = gui::LayoutProblem;
    return problem == Problem::unknown_custom || problem == Problem::unknown_drawable;
}

/// "default.png" to "default": how an atlas names a packed page.
[[nodiscard]] std::string region_name_of( std::string_view file )
{
    const std::size_t dot = file.rfind( '.' );
    return std::string( dot == std::string_view::npos ? file : file.substr( 0, dot ) );
}

[[nodiscard]] bool named_below( const gui::Layout *layout, std::string_view name ) noexcept
{
    return layout->name() < name;
}

/// A layout's name, for a line: "(unnamed)" for a C++ one without.
[[nodiscard]] std::string_view name_of( const gui::Layout &layout ) noexcept
{
    return layout.name().empty() ? std::string_view( "(unnamed)" ) : layout.name();
}

} // namespace

/// What a window has. Made whole or not at all: the members are made in
/// order, the skin loaded into the widgets' own before the painter, which
/// needs its font, is made; `loaded` says whether it all came.
struct Stage::Drawing final
{
        Drawing( files::FileSystem &files, std::string &why );

        graphics::TextureAtlas atlas;
        gui::FontPtr font;
        graphics::SpriteBatch batch;
        gui::Ui ui{ logical_short_side, logical_short_side };
        bool loaded;
        graphics::OrthographicCamera camera{ logical_short_side, logical_short_side };
        gui::BatchPainter painter{ batch, font.get() };

    private:
        /// Load the skin from `files` into the widgets': false, with `why`, when it cannot be.
        [[nodiscard]] bool load( files::FileSystem &files, std::string &why );
        [[nodiscard]] bool load_font( files::FileSystem &files, std::string &why );
};

Stage::Drawing::Drawing( files::FileSystem &files, std::string &why )
    : loaded( load( files, why ) )
{
    // Fills from the skin's white region share the atlas page with every
    // widget: no texture switch, so no extra draw call.
    const graphics::AtlasRegion *white = atlas.find_region( "white" );
    if( loaded && !painter.set_fill_source( white != nullptr ? &white->region() : nullptr ) ) {
        why = "the skin has no white region to fill with";
        loaded = false;
    }
}

bool Stage::Drawing::load_font( files::FileSystem &files, std::string &why )
{
    const std::string path = std::string( skin_directory ) + "/default.fnt";
    const files::FileTextResult text = files.internal( path ).read_string();
    if( !text ) {
        why = path + " could not be read";
        return false;
    }
    graphics::BitmapFont bitmap;
    if( const auto error = bitmap.load( text.text ) ) {
        why = path + " line " + std::to_string( error.line ) + ": " + error.message;
        return false;
    }
    // A skin packs the font's page into its atlas, under the page file's stem.
    std::vector<graphics::TextureRegion> pages;
    for( const graphics::BitmapFontPage &page : bitmap.pages() ) {
        const graphics::AtlasRegion *packed = atlas.find_region( region_name_of( page.file ) );
        if( packed == nullptr ) {
            why = "the font's page is not in the atlas: " + page.file;
            return false;
        }
        pages.push_back( packed->region() );
    }
    font = std::make_shared<gui::FontResource>( "default.fnt", std::move( bitmap ),
                                                std::move( pages ) );
    if( !font->resolved() ) {
        why = path + ": the font did not resolve";
        return false;
    }
    return true;
}

bool Stage::Drawing::load( files::FileSystem &files, std::string &why )
{
    const std::string directory( skin_directory );
    if( !atlas.load( files.internal( directory + "/uiskin.atlas" ) ) ) {
        why = directory + "/uiskin.atlas could not be loaded";
        return false;
    }
    if( !load_font( files, why ) ) {
        return false;
    }
    const files::FileTextResult json = files.internal( directory + "/uiskin.json" ).read_string();
    if( !json ) {
        why = directory + "/uiskin.json could not be read";
        return false;
    }
    gui::SkinLoadReport report;
    const auto drawables = [this]( std::string_view name ) {
        return gui::resolve_atlas_drawable( atlas, name );
    };
    const auto fonts = [this]( std::string_view, std::string_view ) { return font; };
    if( !gui::load_libgdx_skin( ui.skin(), json.text, drawables, fonts, report ) ) {
        why = directory + "/uiskin.json:";
        for( const gui::SkinLoadIssue &issue : report.issues ) {
            if( issue.severity == gui::SkinLoadSeverity::error ) {
                why += " " + issue.message;
                break;
            }
        }
        return false;
    }
    if( !batch.initialize() ) {
        why = "the sprite batch could not be made";
        return false;
    }
    return true;
}

Stage::Stage( messaging::TelegraphUi &telegraph ) noexcept
    : telegraph_( telegraph )
{}

Stage::~Stage() = default;

gui::Ui *Stage::ui() noexcept
{
    return drawing_ != nullptr ? &drawing_->ui : nullptr;
}

void Stage::say( const char *format, ... ) noexcept
{
    std::array<char, 256> line{};
    va_list arguments;
    va_start( arguments, format );
    const int written = std::vsnprintf( line.data(), line.size(), format, arguments );
    va_end( arguments );
    if( written < 0 ) {
        return;
    }
    const auto length = std::min( static_cast<std::size_t>( written ), line.size() - 1 );
    report_( std::string_view( line.data(), length ) );
}

// --- the window ------------------------------------------------------------

void Stage::attach( app::Runtime &runtime )
{
    drawing_.reset();
    failed_ = false;
    // The one allocation a window makes here, besides what the skin holds:
    // the skin's pieces, the batch, the camera, the painter and the widgets.
    // The widgets hold standard containers, whose sizes are the library's:
    // libc++ (Android, Termux) and libstdc++ (desktop Linux) differ.
#if defined( _LIBCPP_VERSION )
    static_assert( sizeof( Drawing ) == 1648 );
#else
    static_assert( sizeof( Drawing ) == 1832 );
#endif
    std::string why;
    auto drawing = std::make_unique<Drawing>( runtime.files, why );
    if( !drawing->loaded ) {
        failed_ = true;
        say( "stage: the skin did not load: %s", why.c_str() );
        return;
    }
    drawing_ = std::move( drawing );
    drawing_->ui.set_text_input_service( text_input_ );
    resize( runtime.graphics.pixel_width(), runtime.graphics.pixel_height() );
    show_top();
}

void Stage::detach() noexcept
{
    drawing_.reset();
}

void Stage::clear() noexcept
{
    depth_ = 0;
    pending_count_ = 0;
    broken_ = false;
    leave_ = false;
    screens_.fill( nullptr );
    if( drawing_ != nullptr ) {
        show_top();
    }
}

void Stage::set_text_input_service( app::TextInputService *service ) noexcept
{
    text_input_ = service;
    if( gui::Ui *widgets = ui() ) {
        widgets->set_text_input_service( service );
    }
}

void Stage::resize( int pixel_width, int pixel_height ) noexcept
{
    pixel_width_ = pixel_width;
    pixel_height_ = pixel_height;
    const float width = static_cast<float>( pixel_width );
    const float height = static_cast<float>( pixel_height );
    if( pixel_width <= 0 || pixel_height <= 0 ) {
        logical_width_ = logical_short_side;
        logical_height_ = logical_short_side * 16.0F / 9.0F;
    } else if( width <= height ) {
        logical_width_ = logical_short_side;
        logical_height_ = logical_short_side * height / width;
    } else {
        logical_width_ = logical_short_side * width / height;
        logical_height_ = logical_short_side;
    }
    if( drawing_ == nullptr ) {
        return;
    }
    drawing_->camera.set_viewport( logical_width_, logical_height_ );
    // set_viewport leaves the position: centring it puts (0,0) at the top left.
    drawing_->camera.set_position( logical_width_ * 0.5F, logical_height_ * 0.5F );
    drawing_->camera.update();
    drawing_->painter.set_viewport( logical_width_, logical_height_, pixel_width, pixel_height );
    drawing_->ui.resize( logical_width_, logical_height_ );
    set_safe_area( safe_area_ );
}

void Stage::set_safe_area( const app::SafeArea &area ) noexcept
{
    safe_area_ = area;
    gui::Ui *widgets = ui();
    if( widgets == nullptr || pixel_width_ <= 0 || pixel_height_ <= 0 ) {
        return;
    }
    // Measured in pixels; the widgets live in logical units.
    const float x = logical_width_ / static_cast<float>( pixel_width_ );
    const float y = logical_height_ / static_cast<float>( pixel_height_ );
    const float left = static_cast<float>( area.left ) * x;
    const float top = static_cast<float>( area.top ) * y;
    const float right = static_cast<float>( area.right ) * x;
    const float bottom = static_cast<float>( area.bottom ) * y;
    widgets->set_safe_area( gui::Insets{ left, top, right, bottom } );
    lay_out();
}

void Stage::surface_created( graphics::Context &graphics ) noexcept
{
    if( drawing_ == nullptr ) {
        return;
    }
    // A first context loses nothing, and restore() knows it.
    if( !drawing_->atlas.restore( graphics ) || !drawing_->batch.restore( graphics ) ) {
        say( "stage: the skin's textures did not come back" );
    }
    resize( graphics.pixel_width(), graphics.pixel_height() );
    if( !graphics.resources_preserved() ) {
        show_top();
    }
}

// --- frames ----------------------------------------------------------------

bool Stage::event( const app::Event &event )
{
    using Type = app::Event::Type;
    gui::Ui *widgets = ui();
    if( widgets == nullptr ) {
        return false;
    }
    if( event.type == Type::Resize ) {
        return true; // the platform calls resize() with the pixels
    }
    app::Event translated = event;
    if( is_pointer( event.type ) && pixel_width_ > 0 && pixel_height_ > 0 ) {
        translated.x = event.x * logical_width_ / static_cast<float>( pixel_width_ );
        translated.y = event.y * logical_height_ / static_cast<float>( pixel_height_ );
    }
    dispatching_ = true;
    const bool handled = widgets->event( translated );
    dispatching_ = false;
    flush();
    if( handled ) {
        return true;
    }
    if( event.type == Type::BackRequested ) {
        return back();
    }
    return false;
}

void Stage::update( std::chrono::nanoseconds delta ) noexcept
{
    static_cast<void>( telegraph_.drain() );
    if( gui::Ui *widgets = ui() ) {
        widgets->update( std::chrono::duration<double>( delta ).count() );
    }
}

void Stage::render( graphics::Context &graphics ) noexcept
{
    if( failed_ || broken_ || drawing_ == nullptr ) {
        graphics.clear( dark_red );
        return;
    }
    graphics.clear( slate );
    if( !drawing_->batch.begin( drawing_->camera ) ) {
        return;
    }
    drawing_->ui.paint( drawing_->painter );
    drawing_->batch.end();
}

// --- the stack -------------------------------------------------------------

const gui::Layout *Stage::find( std::string_view name ) noexcept
{
    const std::span<const gui::Layout *const> all = layouts::all();
    const auto found = std::lower_bound( all.begin(), all.end(), name, named_below );
    if( found == all.end() || ( *found )->name() != name ) {
        return nullptr;
    }
    return *found;
}

bool Stage::push( const gui::Layout &layout ) noexcept
{
    if( depth() >= max_depth ) {
        say( "stage: push %.*s: the stack holds %zu screens",
             static_cast<int>( name_of( layout ).size() ), name_of( layout ).data(), max_depth );
        return false;
    }
    change( gui::TapKind::push, &layout );
    return true;
}

void Stage::replace( const gui::Layout &layout ) noexcept
{
    change( gui::TapKind::replace, &layout );
}

bool Stage::back() noexcept
{
    if( depth() <= 1 ) {
        return false;
    }
    change( gui::TapKind::back, nullptr );
    return true;
}

std::size_t Stage::depth() const noexcept
{
    std::size_t depth = depth_;
    for( std::size_t i = 0; i < pending_count_; ++i ) {
        switch( pending_[i].kind ) {
            case gui::TapKind::push:
                ++depth;
                break;
            case gui::TapKind::replace:
                depth = depth == 0 ? 1 : depth;
                break;
            case gui::TapKind::back:
                depth = depth > 1 ? depth - 1 : depth;
                break;
            default:
                break;
        }
    }
    return depth;
}

const gui::Layout *Stage::top() const noexcept
{
    return depth_ == 0 ? nullptr : screens_[depth_ - 1];
}

void Stage::change( gui::TapKind kind, const gui::Layout *layout ) noexcept
{
    if( !dispatching_ ) {
        apply( { kind, layout } );
        return;
    }
    if( pending_count_ == pending_.size() ) {
        say( "stage: more than %zu navigations in one event; the rest are dropped",
             pending_.size() );
        return;
    }
    pending_[pending_count_++] = { kind, layout };
}

void Stage::apply( Change change ) noexcept
{
    switch( change.kind ) {
        case gui::TapKind::push:
            if( depth_ == max_depth ) {
                say( "stage: push: the stack holds %zu screens", max_depth );
                return;
            }
            screens_[depth_++] = change.layout;
            break;
        case gui::TapKind::replace:
            if( depth_ == 0 ) {
                depth_ = 1;
            }
            screens_[depth_ - 1] = change.layout;
            break;
        case gui::TapKind::back:
            if( depth_ <= 1 ) {
                return;
            }
            screens_[--depth_] = nullptr;
            break;
        default:
            return;
    }
    show_top();
}

void Stage::flush() noexcept
{
    // Taken first: a change can ask for another, which then waits for none.
    std::array<Change, 4> changes = pending_;
    const std::size_t count = pending_count_;
    pending_count_ = 0;
    for( std::size_t i = 0; i < count; ++i ) {
        apply( changes[i] );
    }
}

void Stage::show_top() noexcept
{
    broken_ = false;
    gui::Ui *widgets = ui();
    if( widgets == nullptr ) {
        return;
    }
    auto table = std::make_unique<gui::Table>();
    gui::Table &root = *table;
    static_cast<void>( widgets->set_content( std::move( table ) ) );
    const gui::Layout *layout = top();
    if( layout == nullptr ) {
        return;
    }
    const gui::LayoutHooks hooks{ .tapped = &Stage::tapped, .context = this };
    const gui::LayoutCheck built = gui::instantiate( *layout, root, widgets->skin(), hooks );
    if( !built.ok() ) {
        const std::string_view name = name_of( *layout );
        const std::string_view problem = gui::problem_text( built.problem );
        say( "stage: %.*s: %.*s (node %u)", static_cast<int>( name.size() ), name.data(),
             static_cast<int>( problem.size() ), problem.data(),
             static_cast<unsigned>( built.node ) );
        broken_ = !builds_short( built.problem );
    }
    lay_out();
}

void Stage::lay_out() noexcept
{
    gui::Ui *widgets = ui();
    if( widgets != nullptr && widgets->content() != nullptr ) {
        widgets->layout( drawing_->painter );
    }
}

// --- taps ------------------------------------------------------------------

void Stage::tapped( void *context, std::string_view line ) noexcept
{
    static_cast<Stage *>( context )->tap( line );
}

void Stage::tap( std::string_view line ) noexcept
{
    const gui::Tap read = gui::read_tap( line );
    switch( read.kind ) {
        case gui::TapKind::telegram:
            send( line );
            return;
        case gui::TapKind::push:
        case gui::TapKind::replace: {
            const gui::Layout *to = find( read.target );
            if( to == nullptr ) {
                say( "stage: tap %.*s: no layout is called that", static_cast<int>( line.size() ),
                     line.data() );
                return;
            }
            if( read.kind == gui::TapKind::push ) {
                static_cast<void>( push( *to ) );
            } else {
                replace( *to );
            }
            return;
        }
        case gui::TapKind::back:
            // At the root, what the back button does there: leave.
            if( !back() ) {
                leave_ = true;
            }
            return;
        case gui::TapKind::quit:
            leave_ = true;
            return;
        case gui::TapKind::open:
        case gui::TapKind::close:
            say( "stage: tap %.*s: panes come in 7.3", static_cast<int>( line.size() ),
                 line.data() );
            return;
        case gui::TapKind::none:
        case gui::TapKind::invalid:
            say( "stage: tap %.*s: not a tap", static_cast<int>( line.size() ), line.data() );
            return;
    }
}

void Stage::send( std::string_view line ) noexcept
{
    std::array<std::byte, 1024> storage{};
    messaging::TelegramData data( storage );
    const std::optional<messaging::Telegram> read = messaging::read_telegram( line, data );
    if( !read.has_value() ) {
        say( "stage: tap %.*s: not a telegram", static_cast<int>( line.size() ), line.data() );
        return;
    }
    const messaging::SendResult sent = telegraph_.send( *read, data.bytes() );
    if( sent == messaging::SendResult::full || sent == messaging::SendResult::too_large ) {
        say( "stage: tap %.*s: the Telegraph had no room; not sent",
             static_cast<int>( line.size() ), line.data() );
    }
}

} // namespace sq
