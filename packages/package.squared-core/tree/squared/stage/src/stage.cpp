#include "detail/catalog.hpp"
#include "detail/default_skin.hpp"

#include <squared/app/runtime.hpp>
#include <squared/files/embedded_file_system.hpp>
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
#include <squared/gui/check_box.hpp>
#include <squared/gui/instantiate.hpp>
#include <squared/gui/label.hpp>
#include <squared/gui/progress_bar.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/skin_loader.hpp>
#include <squared/gui/slider.hpp>
#include <squared/gui/src/font_ptr.hpp>
#include <squared/gui/src/font_resource.hpp>
#include <squared/gui/src/insets.hpp>
#include <squared/gui/src/size.hpp>
#include <squared/gui/src/size_hints.hpp>
#include <squared/gui/src/skin_load_issue.hpp>
#include <squared/gui/src/skin_load_report.hpp>
#include <squared/gui/src/skin_load_severity.hpp>
#include <squared/gui/table.hpp>
#include <squared/gui/text_field.hpp>
#include <squared/gui/ui.hpp>
#include <squared/gui/window.hpp>
#include <squared/messaging/address.hpp>
#include <squared/messaging/name.hpp>
#include <squared/messaging/telegram.hpp>
#include <squared/messaging/telegram_data.hpp>
#include <squared/messaging/telegram_ring.hpp>
#include <squared/messaging/telegram_text.hpp>
#include <squared/stage/stage.hpp>

#include <algorithm>
#include <cmath>
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

/// Where the default skin is, in the files compiled into the stage library.
constexpr std::string_view skin_directory = "skins/default/skin";

/// The default skin's files, compiled in: one table in read-only data, read
/// through a file system so a texture can reload from it after a lost context.
[[nodiscard]] files::EmbeddedFileSystem &default_skin_files() noexcept
{
    static files::EmbeddedFileSystem files( detail::default_skin() );
    return files;
}

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

/// `Binding::owner` for the screen: a pane's is its slot.
constexpr std::uint8_t screen_owner = 0xFF;

/// The telegram's text, for a label or a text field: text as it is (a view
/// into `data`), any other value as the text form writes it (`42`, `2.0`,
/// `(1.5 2)`), into `scratch`.
[[nodiscard]] std::string_view text_of( const messaging::Telegram &telegram,
                                        std::span<const std::byte> data,
                                        std::span<char> scratch ) noexcept
{
    if( telegram.type() == messaging::ValueType::text ) {
        const messaging::DataRef where = telegram.as_data();
        if( where.offset > data.size() || where.length > data.size() - where.offset ) {
            return {};
        }
        const auto *bytes = reinterpret_cast<const char *>( data.data() ) + where.offset;
        return { bytes, where.length };
    }
    const std::size_t length = messaging::write_value( telegram, data, scratch );
    return { scratch.data(), length };
}

/// A number to show on a slider or a progress bar: an integer or a double.
[[nodiscard]] float number_of( const messaging::Telegram &telegram ) noexcept
{
    if( telegram.type() == messaging::ValueType::integer ) {
        return static_cast<float>( telegram.as_integer() );
    }
    return static_cast<float>( telegram.as_number() );
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

        /// What a C++ screen writes into, and the table it wrote. Last, so
        /// the window's size is the rest plus exactly this.
        struct Scratch final
        {
                std::array<gui::LayoutNode, scratch_nodes> nodes{};
                std::array<char, scratch_text> text{};
                std::array<float, scratch_numbers> numbers{};
                gui::Layout written;
        } scratch;

        /// The bound widgets of the screen and its panes. After the scratch,
        /// so the window's size is the rest plus both.
        std::array<Binding, max_bindings> bindings{};

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
{
    for( Pane &pane : panes_ ) {
        pane.stage = this;
    }
}

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
    // the skin's pieces, the batch, the camera, the painter, the widgets,
    // and the scratch a C++ screen writes into. The widgets hold standard
    // containers, whose sizes are the library's: libc++ (Android, Termux)
    // and libstdc++ (desktop Linux) differ. The scratch is the same in both.
    static_assert( sizeof( Drawing::Scratch ) == 17480 );
    static_assert( sizeof( Binding ) == 32 );
    constexpr std::size_t tables = sizeof( Drawing::Scratch ) + max_bindings * 32;
#if defined( _LIBCPP_VERSION )
    static_assert( sizeof( Drawing ) == 1648 + tables );
#else
    static_assert( sizeof( Drawing ) == 1832 + tables );
#endif
    // The skin is compiled in: the app's own files are not read for it.
    files::FileSystem &skin = skin_files_ != nullptr ? *skin_files_ : default_skin_files();
    std::string why;
    auto drawing = std::make_unique<Drawing>( skin, why );
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
    // The cache may let their values go; then the panes' windows go too.
    unbind_all();
    drawing_.reset();
    for( Pane &pane : panes_ ) {
        pane.window = nullptr;
    }
}

void Stage::clear() noexcept
{
    depth_ = 0;
    pending_count_ = 0;
    broken_ = false;
    leave_ = false;
    screens_.fill( {} );
    drop_panes();
    added_.fill( nullptr );
    customs_.fill( {} );
    added_count_ = 0;
    custom_count_ = 0;
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
    // Each state the model sent goes to the widgets that show it, as it comes.
    struct Showing final
    {
            Stage *stage;
            void state( const messaging::Received &received ) noexcept
            {
                stage->changed( received );
            }
    };
    Showing showing{ this };
    static_cast<void>( telegraph_.drain( showing ) );
    send_edits();
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
    // A name is said with its size beside it, in the same call: the view
    // ends where it says, not at a '\0'.
    const std::string_view name = name_of( layout );
    if( layout.kind() == gui::LayoutKind::pane ) {
        say( "stage: push %.*s: a pane: open it", static_cast<int>( name.size() ), name.data() );
        return false;
    }
    if( depth() >= max_depth ) {
        say( "stage: push %.*s: the stack holds %zu screens", static_cast<int>( name.size() ),
             name.data(), max_depth );
        return false;
    }
    change( { gui::TapKind::push, top_pane, { &layout, nullptr } } );
    return true;
}

bool Stage::push_screen( const detail::ScreenEntry &screen ) noexcept
{
    if( depth() >= max_depth ) {
        say( "stage: push %.*s: the stack holds %zu screens",
             static_cast<int>( screen.name.size() ), screen.name.data(), max_depth );
        return false;
    }
    change( { gui::TapKind::push, top_pane, { nullptr, &screen } } );
    return true;
}

bool Stage::replace( const gui::Layout &layout ) noexcept
{
    const std::string_view name = name_of( layout );
    if( layout.kind() == gui::LayoutKind::pane ) {
        say( "stage: replace %.*s: a pane: open it", static_cast<int>( name.size() ), name.data() );
        return false;
    }
    change( { gui::TapKind::replace, top_pane, { &layout, nullptr } } );
    return true;
}

bool Stage::back() noexcept
{
    const Counts counts = after();
    if( counts.panes == 0 && counts.depth <= 1 ) {
        return false;
    }
    change( { gui::TapKind::back, top_pane, {} } );
    return true;
}

bool Stage::open( const gui::Layout &layout ) noexcept
{
    const std::string_view name = name_of( layout );
    if( layout.kind() != gui::LayoutKind::pane ) {
        say( "stage: open %.*s: a screen: push it", static_cast<int>( name.size() ), name.data() );
        return false;
    }
    if( after().panes >= max_panes ) {
        say( "stage: open %.*s: %zu panes are open already", static_cast<int>( name.size() ),
             name.data(), max_panes );
        return false;
    }
    change( { gui::TapKind::open, top_pane, { &layout, nullptr } } );
    return true;
}

bool Stage::open_screen( const detail::ScreenEntry &screen ) noexcept
{
    if( after().panes >= max_panes ) {
        say( "stage: open %.*s: %zu panes are open already", static_cast<int>( screen.name.size() ),
             screen.name.data(), max_panes );
        return false;
    }
    change( { gui::TapKind::open, top_pane, { nullptr, &screen } } );
    return true;
}

bool Stage::close() noexcept
{
    if( after().panes == 0 ) {
        return false;
    }
    change( { gui::TapKind::close, top_pane, {} } );
    return true;
}

Stage::Counts Stage::after() const noexcept
{
    Counts counts{ depth_, pane_count_ };
    for( std::size_t i = 0; i < pending_count_; ++i ) {
        switch( pending_[i].kind ) {
            case gui::TapKind::push:
                ++counts.depth;
                counts.panes = 0;
                break;
            case gui::TapKind::replace:
                counts.depth = std::max<std::size_t>( counts.depth, 1 );
                counts.panes = 0;
                break;
            case gui::TapKind::back:
                if( counts.panes > 0 ) {
                    --counts.panes;
                } else if( counts.depth > 1 ) {
                    --counts.depth;
                }
                break;
            case gui::TapKind::open:
                counts.panes = std::min( counts.panes + 1, max_panes );
                break;
            case gui::TapKind::close:
                counts.panes = counts.panes > 0 ? counts.panes - 1 : 0;
                break;
            default:
                break;
        }
    }
    return counts;
}

std::size_t Stage::depth() const noexcept
{
    return after().depth;
}

std::size_t Stage::panes() const noexcept
{
    return after().panes;
}

const gui::Layout *Stage::top() const noexcept
{
    if( depth_ == 0 ) {
        return nullptr;
    }
    const Shown &shown = screens_[depth_ - 1];
    if( shown.layout != nullptr ) {
        return shown.layout;
    }
    return drawing_ != nullptr ? &drawing_->scratch.written : nullptr;
}

std::string_view Stage::top_name() const noexcept
{
    if( depth_ == 0 ) {
        return {};
    }
    const Shown &shown = screens_[depth_ - 1];
    return shown.layout != nullptr ? shown.layout->name() : shown.screen->name;
}

std::string_view Stage::pane_name() const noexcept
{
    const Pane *pane = top_pane_slot();
    if( pane == nullptr ) {
        return {};
    }
    return pane->shown.layout != nullptr ? pane->shown.layout->name() : pane->shown.screen->name;
}

// --- names -----------------------------------------------------------------

const detail::ScreenEntry *Stage::added( std::string_view name ) const noexcept
{
    for( std::size_t i = 0; i < added_count_; ++i ) {
        if( added_[i]->name == name ) {
            return added_[i];
        }
    }
    return nullptr;
}

Stage::Shown Stage::resolve( std::string_view name ) const noexcept
{
    if( const gui::Layout *layout = find( name ) ) {
        return { layout, nullptr };
    }
    return { nullptr, added( name ) };
}

bool Stage::add_screen( const detail::ScreenEntry &screen ) noexcept
{
    const auto name_size = static_cast<int>( screen.name.size() );
    const char *name = screen.name.data();
    if( !messaging::valid_name( screen.name ) ) {
        say( "stage: add %.*s: not a name: lowercase words, digits and _", name_size, name );
        return false;
    }
    if( const detail::ScreenEntry *there = added( screen.name ) ) {
        if( there == &screen ) {
            return true;
        }
        say( "stage: add %.*s: another screen has that name", name_size, name );
        return false;
    }
    if( find( screen.name ) != nullptr ) {
        say( "stage: add %.*s: a layout file has that name", name_size, name );
        return false;
    }
    if( added_count_ == max_added ) {
        say( "stage: add %.*s: %zu screens are added already", name_size, name, max_added );
        return false;
    }
    added_[added_count_++] = &screen;
    return true;
}

bool Stage::custom( std::string_view name, CustomMaker make ) noexcept
{
    if( name.empty() || make == nullptr ) {
        say( "stage: custom %.*s: a name and a maker", static_cast<int>( name.size() ),
             name.data() );
        return false;
    }
    for( std::size_t i = 0; i < custom_count_; ++i ) {
        if( customs_[i].name == name ) {
            if( customs_[i].make == make ) {
                return true;
            }
            say( "stage: custom %.*s: another maker has that name", static_cast<int>( name.size() ),
                 name.data() );
            return false;
        }
    }
    if( custom_count_ == max_added ) {
        say( "stage: custom %.*s: %zu types are there already", static_cast<int>( name.size() ),
             name.data(), max_added );
        return false;
    }
    customs_[custom_count_++] = { name, make };
    return true;
}

void Stage::check_screens() noexcept
{
    for( const std::string_view name : layouts::screens() ) {
        if( added( name ) == nullptr ) {
            say( "stage: names.json lists the screen %.*s, and it is not added: "
                 "app.stage.add<...>() in squared_main",
                 static_cast<int>( name.size() ), name.data() );
        }
    }
}

// --- the stack -------------------------------------------------------------

void Stage::change( Change change ) noexcept
{
    if( !dispatching_ ) {
        apply( change );
        return;
    }
    if( pending_count_ == pending_.size() ) {
        say( "stage: more than %zu navigations in one event; the rest are dropped",
             pending_.size() );
        return;
    }
    pending_[pending_count_++] = change;
}

void Stage::apply( Change change ) noexcept
{
    switch( change.kind ) {
        case gui::TapKind::push:
            if( depth_ == max_depth ) {
                say( "stage: push: the stack holds %zu screens", max_depth );
                return;
            }
            screens_[depth_++] = change.shown;
            break;
        case gui::TapKind::replace:
            if( depth_ == 0 ) {
                depth_ = 1;
            }
            screens_[depth_ - 1] = change.shown;
            break;
        case gui::TapKind::back:
            if( Pane *pane = top_pane_slot() ) {
                close_pane( *pane ); // the back button closes the top pane first
                return;
            }
            if( depth_ <= 1 ) {
                return;
            }
            screens_[--depth_] = {};
            break;
        case gui::TapKind::open:
            open_pane( change.shown );
            return;
        case gui::TapKind::close:
            if( change.pane == top_pane ) {
                if( Pane *pane = top_pane_slot() ) {
                    close_pane( *pane );
                }
            } else if( change.pane < panes_.size() && panes_[change.pane].order != 0 ) {
                close_pane( panes_[change.pane] );
            }
            return;
        default:
            return;
    }
    // A screen left takes its panes with it.
    drop_panes();
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
    // The bound widgets go with the old content, and the panes' windows: the
    // ones that stay are built again below.
    unbind_all();
    static_cast<void>( widgets->set_content( std::move( table ) ) );
    for( Pane &pane : panes_ ) {
        pane.window = nullptr;
    }
    if( depth_ == 0 ) {
        return;
    }
    const Shown &shown = screens_[depth_ - 1];
    if( shown.screen != nullptr ) {
        write( *shown.screen, gui::LayoutKind::screen );
    }
    const gui::Layout *layout = top();
    gui::LayoutHooks hooks;
    hooks.tapped = &Stage::tapped;
    hooks.bound = &Stage::bound;
    hooks.custom = &Stage::made;
    hooks.context = this;
    const gui::LayoutCheck check = gui::instantiate( *layout, root, widgets->skin(), hooks );
    broken_ = !built( *layout, check );
    for( std::uint8_t order = 1; order <= pane_count_; ++order ) {
        for( Pane &pane : panes_ ) {
            if( pane.order == order ) {
                build_pane( pane );
            }
        }
    }
    lay_out();
}

bool Stage::built( const gui::Layout &layout, gui::LayoutCheck check ) noexcept
{
    if( check.ok() ) {
        return true;
    }
    const std::string_view name = name_of( layout );
    const std::string_view problem = gui::problem_text( check.problem );
    say( "stage: %.*s: %.*s (node %u)", static_cast<int>( name.size() ), name.data(),
         static_cast<int>( problem.size() ), problem.data(), static_cast<unsigned>( check.node ) );
    return builds_short( check.problem );
}

void Stage::write( const detail::ScreenEntry &screen, gui::LayoutKind kind ) noexcept
{
    Drawing::Scratch &scratch = drawing_->scratch;
    gui::LayoutWriter writer( scratch.nodes, scratch.text, scratch.numbers, screen.name, kind );
    writer.set_pane( screen.pane );
    screen.build( writer );
    if( !writer.complete() ) {
        say( "stage: %.*s: not all of it was written: a call was refused, or the scratch "
             "(%zu nodes, %zu bytes of text, %zu numbers) is full",
             static_cast<int>( screen.name.size() ), screen.name.data(), scratch_nodes,
             scratch_text, scratch_numbers );
    }
    scratch.written = writer.layout();
}

void Stage::lay_out() noexcept
{
    gui::Ui *widgets = ui();
    if( widgets != nullptr && widgets->content() != nullptr ) {
        place_panes();
        widgets->layout( drawing_->painter );
    }
}

// --- panes -----------------------------------------------------------------

Stage::Pane *Stage::top_pane_slot() noexcept
{
    for( Pane &pane : panes_ ) {
        if( pane.order != 0 && pane.order == pane_count_ ) {
            return &pane;
        }
    }
    return nullptr;
}

const Stage::Pane *Stage::top_pane_slot() const noexcept
{
    for( const Pane &pane : panes_ ) {
        if( pane.order != 0 && pane.order == pane_count_ ) {
            return &pane;
        }
    }
    return nullptr;
}

void Stage::open_pane( Shown shown ) noexcept
{
    if( depth_ == 0 ) {
        say( "stage: open: no screen to open a pane over" );
        return;
    }
    for( Pane &pane : panes_ ) {
        if( pane.order != 0 ) {
            continue;
        }
        pane.shown = shown;
        pane.order = ++pane_count_;
        build_pane( pane );
        lay_out();
        return;
    }
    say( "stage: open: %zu panes are open already", max_panes );
}

void Stage::build_pane( Pane &pane ) noexcept
{
    gui::Ui *widgets = ui();
    if( widgets == nullptr ) {
        return; // built when the window comes
    }
    if( pane.shown.screen != nullptr ) {
        write( *pane.shown.screen, gui::LayoutKind::pane );
    }
    const gui::Layout *layout = pane.shown.layout;
    if( layout == nullptr ) {
        layout = &drawing_->scratch.written;
    }
    // Over a modal pane every pane is modal too: what is beneath one takes
    // no taps, so a pane over it would take none either.
    bool modal = layout->pane().modal;
    for( const Pane &below : panes_ ) {
        if( below.order != 0 && below.order < pane.order && below.window != nullptr ) {
            modal = modal || below.window->modal();
        }
    }
    auto window = std::make_unique<gui::Window>();
    window->set_titled( false );
    window->set_movable( false );
    window->set_modal( modal );
    gui::LayoutHooks hooks;
    hooks.tapped = &Stage::tapped_in_pane;
    hooks.bound = &Stage::bound_in_pane;
    hooks.custom = &Stage::made_in_pane;
    hooks.context = &pane;
    gui::Table &table = window->content_table();
    const gui::LayoutCheck check = gui::instantiate( *layout, table, widgets->skin(), hooks );
    // A pane that did not build still opens, empty, so back closes it.
    static_cast<void>( built( *layout, check ) );
    pane.place = layout->pane().place;
    pane.window = &widgets->show_window( std::move( window ), false );
}

void Stage::close_pane( Pane &pane ) noexcept
{
    unbind_owner( static_cast<std::uint8_t>( &pane - panes_.data() ) );
    if( pane.window != nullptr ) {
        if( gui::Ui *widgets = ui() ) {
            widgets->close_window( *pane.window ); // frees its widgets
        }
    }
    for( Pane &other : panes_ ) {
        if( other.order > pane.order ) {
            --other.order;
        }
    }
    --pane_count_;
    pane.shown = {};
    pane.window = nullptr;
    pane.order = 0;
}

void Stage::drop_panes() noexcept
{
    // Their windows, if built, go with the screen's content.
    for( Pane &pane : panes_ ) {
        pane.shown = {};
        pane.window = nullptr;
        pane.order = 0;
    }
    pane_count_ = 0;
}

void Stage::place_panes() noexcept
{
    gui::Ui &widgets = drawing_->ui;
    const gui::Insets safe = widgets.safe_area();
    const float left = safe.left;
    const float top = safe.top;
    const float across = std::max( 0.0F, logical_width_ - safe.left - safe.right );
    const float down = std::max( 0.0F, logical_height_ - safe.top - safe.bottom );
    for( Pane &pane : panes_ ) {
        if( pane.window == nullptr ) {
            continue;
        }
        gui::Window &window = *pane.window;
        const gui::Size wanted = window.size_hints( drawing_->painter, widgets.skin() ).preferred;
        float width = std::min( wanted.width, across );
        float height = std::min( wanted.height, down );
        float x = left + ( across - width ) * 0.5F;
        float y = top + ( down - height ) * 0.5F;
        switch( pane.place ) {
            case gui::PanePlace::center:
                break;
            case gui::PanePlace::left:
            case gui::PanePlace::right:
                x = pane.place == gui::PanePlace::left ? left : left + across - width;
                y = top;
                height = down;
                break;
            case gui::PanePlace::top:
            case gui::PanePlace::bottom:
                x = left;
                y = pane.place == gui::PanePlace::top ? top : top + down - height;
                width = across;
                break;
        }
        window.set_bounds( x, y, width, height );
        window.invalidate_layout();
    }
}

// --- bound widgets ---------------------------------------------------------

std::size_t Stage::bindings() const noexcept
{
    if( drawing_ == nullptr ) {
        return 0;
    }
    std::size_t count = 0;
    for( const Binding &binding : drawing_->bindings ) {
        count += binding.widget != nullptr ? 1 : 0;
    }
    return count;
}

void Stage::bound( void *context, const gui::Layout &layout, const gui::LayoutNode &node,
                   gui::Widget &widget ) noexcept
{
    static_cast<Stage *>( context )->bind( screen_owner, layout, node, widget );
}

void Stage::bound_in_pane( void *context, const gui::Layout &layout, const gui::LayoutNode &node,
                           gui::Widget &widget ) noexcept
{
    Pane &pane = *static_cast<Pane *>( context );
    const auto slot = static_cast<std::uint8_t>( &pane - pane.stage->panes_.data() );
    pane.stage->bind( slot, layout, node, widget );
}

void Stage::bind( std::uint8_t owner, const gui::Layout &layout, const gui::LayoutNode &node,
                  gui::Widget &widget ) noexcept
{
    Binding *slot = nullptr;
    for( Binding &binding : drawing_->bindings ) {
        if( binding.widget == nullptr ) {
            slot = &binding;
            break;
        }
    }
    if( slot == nullptr ) {
        if( !bindings_full_ ) {
            const std::string_view name = name_of( layout );
            say( "stage: %.*s: more than %zu bound values; the rest show their own text",
                 static_cast<int>( name.size() ), name.data(), max_bindings );
            bindings_full_ = true;
        }
        return;
    }
    *slot = {};
    slot->address = { node.bind, 0 };
    slot->widget = &widget;
    slot->type = node.type;
    slot->value = node.value;
    slot->owner = owner;
    slot->sends_set = node.sends_set;
    // Its value stays in the cache while it is shown.
    static_cast<void>( telegraph_.bind( slot->address ) );
    // The widgets' callbacks are std::function (the gui's, from before):
    // two pointers fit in its own storage, so this allocates nothing.
    const auto toggled = [this, &widget]( bool on ) { edited( widget, on ); };
    const auto moved = [this, &widget]( float value ) { edited( widget, value ); };
    if( node.sends_set ) {
        switch( node.type ) {
            case gui::NodeType::check_box:
                static_cast<gui::CheckBox &>( widget ).set_on_change( toggled );
                break;
            case gui::NodeType::slider:
                static_cast<gui::Slider &>( widget ).set_on_change( moved );
                break;
            case gui::NodeType::text_field:
                static_cast<gui::TextField &>( widget ).set_on_commit( &Stage::committed, this );
                break;
            default:
                break;
        }
    }
    if( const std::optional<messaging::CachedValue> now = telegraph_.find( slot->address ) ) {
        show( *slot, now->telegram, now->data );
    }
}

void Stage::unbind_all() noexcept
{
    if( drawing_ == nullptr ) {
        return;
    }
    for( Binding &binding : drawing_->bindings ) {
        if( binding.widget != nullptr ) {
            telegraph_.unbind( binding.address );
            binding = {};
        }
    }
    bindings_full_ = false;
}

void Stage::unbind_owner( std::uint8_t owner ) noexcept
{
    if( drawing_ == nullptr ) {
        return;
    }
    for( Binding &binding : drawing_->bindings ) {
        if( binding.widget != nullptr && binding.owner == owner ) {
            telegraph_.unbind( binding.address );
            binding = {};
        }
    }
}

bool Stage::held( const Binding &binding ) noexcept
{
    switch( binding.type ) {
        case gui::NodeType::slider:
            return static_cast<const gui::Slider *>( binding.widget )->dragging();
        case gui::NodeType::text_field:
            return static_cast<const gui::TextField *>( binding.widget )->focused();
        default:
            return false;
    }
}

void Stage::show( Binding &binding, const messaging::Telegram &telegram,
                  std::span<const std::byte> data ) noexcept
{
    if( telegram.type() != binding.value ) {
        return; // not the type it was bound as: a model's key and names.json disagree
    }
    binding.stale = false;
    // A value's text, when it is not text already: the longest, four floats.
    std::array<char, 128> scratch{};
    std::string_view text;
    if( binding.type == gui::NodeType::label || binding.type == gui::NodeType::text_field ) {
        text = text_of( telegram, data, scratch );
    }
    // The widgets tell of changes made here as of the user's: not sent back.
    applying_ = true;
    switch( binding.type ) {
        case gui::NodeType::label:
            static_cast<gui::Label *>( binding.widget )->set_text( text );
            break;
        case gui::NodeType::check_box:
            static_cast<gui::CheckBox *>( binding.widget )->set_checked( telegram.as_boolean() );
            break;
        case gui::NodeType::slider:
            static_cast<gui::Slider *>( binding.widget )->set_value( number_of( telegram ) );
            break;
        case gui::NodeType::progress_bar:
            static_cast<gui::ProgressBar *>( binding.widget )->set_value( number_of( telegram ) );
            break;
        case gui::NodeType::text_field:
            static_cast<gui::TextField *>( binding.widget )->set_text( text );
            break;
        default:
            break;
    }
    applying_ = false;
}

void Stage::changed( const messaging::Received &received ) noexcept
{
    if( drawing_ == nullptr ) {
        return;
    }
    // Shown from the cache, which has it now: its text is where its
    // reference says, which a telegram straight from the ring's is not.
    const messaging::Address address = received.telegram.address();
    std::optional<messaging::CachedValue> now;
    for( Binding &binding : drawing_->bindings ) {
        if( binding.widget == nullptr || !( binding.address == address ) ) {
            continue;
        }
        if( held( binding ) ) {
            binding.stale = true; // the user has it: shown when they let go
            continue;
        }
        if( !now ) {
            now = telegraph_.find( address );
        }
        if( now ) {
            show( binding, now->telegram, now->data );
        }
    }
}

Stage::Binding *Stage::binding_of( const gui::Widget &widget ) noexcept
{
    if( drawing_ == nullptr ) {
        return nullptr;
    }
    for( Binding &binding : drawing_->bindings ) {
        if( binding.widget == &widget ) {
            return &binding;
        }
    }
    return nullptr;
}

void Stage::edited( gui::Widget &widget, bool on ) noexcept
{
    Binding *binding = applying_ ? nullptr : binding_of( widget );
    if( binding == nullptr || !binding->sends_set ) {
        return;
    }
    set( messaging::Telegram::boolean( messaging::Verb::set, binding->address, on ), {} );
}

void Stage::edited( gui::Widget &widget, float value ) noexcept
{
    Binding *binding = applying_ ? nullptr : binding_of( widget );
    if( binding == nullptr || !binding->sends_set ) {
        return;
    }
    // A drag moves many times a frame: the frame sends the last.
    binding->pending = value;
    binding->sending = true;
}

void Stage::committed( void *context, gui::TextField &field ) noexcept
{
    static_cast<Stage *>( context )->edited( field );
}

void Stage::edited( gui::TextField &field ) noexcept
{
    Binding *binding = applying_ ? nullptr : binding_of( field );
    if( binding == nullptr || !binding->sends_set ) {
        return;
    }
    std::array<std::byte, 1024> storage{};
    messaging::TelegramData data( storage );
    const std::string &text = field.text();
    const auto *bytes = reinterpret_cast<const std::byte *>( text.data() );
    const std::optional<messaging::DataRef> where = data.append( { bytes, text.size() } );
    if( !where ) {
        say( "stage: an edit was not sent: more than %zu bytes of text", storage.size() );
        return;
    }
    const messaging::ValueType type = messaging::ValueType::text;
    set( messaging::Telegram::data( messaging::Verb::set, binding->address, type, *where ),
         data.bytes() );
}

void Stage::send_edits() noexcept
{
    if( drawing_ == nullptr ) {
        return;
    }
    for( Binding &binding : drawing_->bindings ) {
        if( binding.widget == nullptr ) {
            continue;
        }
        if( binding.sending ) {
            binding.sending = false;
            const messaging::Verb verb = messaging::Verb::set;
            if( binding.value == messaging::ValueType::integer ) {
                const auto whole = static_cast<std::int64_t>( std::lround( binding.pending ) );
                set( messaging::Telegram::integer( verb, binding.address, whole ), {} );
            } else {
                const auto number = static_cast<double>( binding.pending );
                set( messaging::Telegram::number( verb, binding.address, number ), {} );
            }
        }
        // Let go: the model's latest value is shown.
        if( binding.stale && !held( binding ) ) {
            const std::optional<messaging::CachedValue> now = telegraph_.find( binding.address );
            if( now ) {
                show( binding, now->telegram, now->data );
            }
        }
    }
}

void Stage::set( const messaging::Telegram &telegram, std::span<const std::byte> data ) noexcept
{
    const messaging::SendResult sent = telegraph_.send( telegram, data );
    if( sent == messaging::SendResult::full || sent == messaging::SendResult::too_large ) {
        say( "stage: an edit was not sent: the Telegraph had no room" );
    }
}

// --- taps ------------------------------------------------------------------

void Stage::tapped( void *context, std::string_view line ) noexcept
{
    static_cast<Stage *>( context )->tap( line );
}

void Stage::tapped_in_pane( void *context, std::string_view line ) noexcept
{
    Pane &pane = *static_cast<Pane *>( context );
    Stage &stage = *pane.stage;
    if( gui::read_tap( line ).kind != gui::TapKind::close ) {
        stage.tap( line );
        return;
    }
    // This pane, whichever is on top.
    const auto slot = static_cast<std::uint8_t>( &pane - stage.panes_.data() );
    stage.change( { gui::TapKind::close, slot, {} } );
}

std::unique_ptr<gui::Widget> Stage::made_in_pane( void *context, std::string_view name )
{
    return made( static_cast<Pane *>( context )->stage, name );
}

std::unique_ptr<gui::Widget> Stage::made( void *context, std::string_view name )
{
    const auto &stage = *static_cast<const Stage *>( context );
    for( std::size_t i = 0; i < stage.custom_count_; ++i ) {
        if( stage.customs_[i].name == name ) {
            return stage.customs_[i].make();
        }
    }
    return nullptr;
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
            const Shown to = resolve( read.target );
            if( to.layout == nullptr && to.screen == nullptr ) {
                say( "stage: tap %.*s: no layout or added screen is called that",
                     static_cast<int>( line.size() ), line.data() );
                return;
            }
            if( read.kind == gui::TapKind::replace && to.layout != nullptr ) {
                static_cast<void>( replace( *to.layout ) );
            } else if( read.kind == gui::TapKind::replace ) {
                change( { gui::TapKind::replace, top_pane, to } );
            } else if( to.layout != nullptr ) {
                static_cast<void>( push( *to.layout ) );
            } else {
                static_cast<void>( push_screen( *to.screen ) );
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
        case gui::TapKind::open: {
            const Shown to = resolve( read.target );
            if( to.layout != nullptr ) {
                static_cast<void>( open( *to.layout ) );
            } else if( to.screen != nullptr ) {
                static_cast<void>( open_screen( *to.screen ) );
            } else {
                say( "stage: tap %.*s: no layout or added screen is called that",
                     static_cast<int>( line.size() ), line.data() );
            }
            return;
        }
        case gui::TapKind::close:
            // A pane's own close comes through tapped_in_pane.
            say( "stage: tap close: this is a screen, not a pane" );
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
