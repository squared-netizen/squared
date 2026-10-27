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
#include <squared/gui/list.hpp>
#include <squared/gui/progress_bar.hpp>
#include <squared/gui/scroll_pane.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/skin_loader.hpp>
#include <squared/gui/slider.hpp>
#include <squared/gui/src/detail/pane_frame.hpp>
#include <squared/gui/src/detail/ui.hpp>
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
#include <squared/messaging/address.hpp>
#include <squared/messaging/list_window.hpp>
#include <squared/messaging/name.hpp>
#include <squared/messaging/telegram.hpp>
#include <squared/messaging/telegram_data.hpp>
#include <squared/messaging/telegram_ring.hpp>
#include <squared/messaging/telegram_text.hpp>
#include <squared/stage/stage.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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

/// A count or a uid from an integer: 0 to `UINT32_MAX`.
[[nodiscard]] std::uint32_t rows_of( std::int64_t value ) noexcept
{
    return static_cast<std::uint32_t>( std::clamp<std::int64_t>( value, 0, UINT32_MAX ) );
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

/// Visit, in the layout's order, the widgets a screen keeps the state of:
/// each list, scroll pane and text field. A list's rows are its own, and
/// change as it scrolls: not visited.
template<typename Visitor>
void walk( gui::Actor &actor, Visitor &visitor ) noexcept
{
    if( auto *list = gui::actor_cast<gui::List>( &actor ) ) {
        visitor.list( *list );
        return;
    }
    if( auto *field = gui::actor_cast<gui::TextField>( &actor ) ) {
        visitor.field( *field );
        return;
    }
    if( auto *pane = gui::actor_cast<gui::ScrollPane>( &actor ) ) {
        visitor.pane( *pane );
    }
    auto *group = gui::actor_cast<gui::Group>( &actor );
    if( group == nullptr ) {
        return;
    }
    for( std::size_t i = 0; i < group->child_count(); ++i ) {
        walk( *group->child_at( i ), visitor );
    }
}

/// What a screen's widgets are, into its `Kept` (`K`): the first scrolls,
/// and the first text field with an edit not finished.
template<typename K>
struct Keeping final
{
        K &kept;
        std::size_t fields = 0;

        void scroll( float offset ) noexcept
        {
            if( kept.scrolls < kept.scroll.size() ) {
                kept.scroll[kept.scrolls++] = offset;
            }
        }
        void list( gui::List &list ) noexcept { scroll( list.scroll_y() ); }
        void pane( gui::ScrollPane &pane ) noexcept { scroll( pane.scroll_y() ); }
        void field( gui::TextField &field ) noexcept
        {
            const std::size_t at = fields++;
            if( kept.field != K::no_field || !field.edited() || at >= K::no_field ) {
                return;
            }
            const std::string_view text = field.text();
            std::size_t length = std::min( text.size(), kept.text.size() );
            // Cut where a character starts, never inside one.
            while( length > 0 && length < text.size() &&
                   ( static_cast<unsigned char>( text[length] ) & 0xC0U ) == 0x80U ) {
                --length;
            }
            const char *from = text.data(); // not text.data() in the call: length is cut
            std::copy_n( from, length, kept.text.data() );
            kept.text_length = static_cast<std::uint8_t>( length );
            kept.field = static_cast<std::uint8_t>( at );
        }
};

/// What was kept, put back into a screen built again: the same widgets, in
/// the same order.
template<typename K>
struct Putting final
{
        const K &kept;
        std::size_t scrolls = 0;
        std::size_t fields = 0;

        [[nodiscard]] std::optional<float> next() noexcept
        {
            const std::size_t at = scrolls++;
            return at < kept.scrolls ? std::optional<float>( kept.scroll[at] ) : std::nullopt;
        }
        void list( gui::List &list ) noexcept
        {
            // Its rows come from the model again: as far as they reach meanwhile.
            if( const std::optional<float> offset = next(); offset && *offset > 0.0F ) {
                list.restore_scroll( *offset );
            }
        }
        void pane( gui::ScrollPane &pane ) noexcept
        {
            if( const std::optional<float> offset = next() ) {
                pane.set_scroll_y( *offset );
            }
        }
        void field( gui::TextField &field ) noexcept
        {
            const std::size_t at = fields++;
            if( kept.field != K::no_field && at == kept.field ) {
                field.restore_edit( { kept.text.data(), kept.text_length } );
            }
        }
};

/// Lines into fixed storage: what the saved stack and `describe` write.
class LineWriter final
{
    public:
        explicit LineWriter( std::span<char> out ) noexcept
            : out_( out )
        {}

        void text( std::string_view text ) noexcept
        {
            if( full_ || text.size() > out_.size() - used_ ) {
                full_ = true;
                return;
            }
            std::copy( text.begin(), text.end(),
                       out_.begin() + static_cast<std::ptrdiff_t>( used_ ) );
            used_ += text.size();
        }

        void number( std::int64_t value ) noexcept
        {
            std::array<char, 24> digits{};
            const std::to_chars_result end =
                std::to_chars( digits.data(), digits.data() + digits.size(), value );
            text( { digits.data(), static_cast<std::size_t>( end.ptr - digits.data() ) } );
        }

        /// Each byte as two lowercase hex digits.
        void hex( std::string_view bytes ) noexcept
        {
            constexpr std::string_view digits = "0123456789abcdef";
            for( const char byte : bytes ) {
                const auto value = static_cast<unsigned char>( byte );
                const std::array<char, 2> pair{ digits[value >> 4U], digits[value & 0x0FU] };
                text( { pair.data(), pair.size() } );
            }
        }

        [[nodiscard]] std::string_view written() const noexcept { return { out_.data(), used_ }; }
        [[nodiscard]] bool full() const noexcept { return full_; }

    private:
        std::span<char> out_;
        std::size_t used_ = 0;
        bool full_ = false;
};

/// The first word of `line`, taken off it: up to a space, or the end.
[[nodiscard]] std::string_view take_word( std::string_view &line ) noexcept
{
    const std::size_t space = line.find( ' ' );
    const std::string_view word = line.substr( 0, space );
    line = space == std::string_view::npos ? std::string_view{} : line.substr( space + 1 );
    return word;
}

/// A whole number from `lowest` to `highest`; nothing when the word is not one.
[[nodiscard]] std::optional<std::int64_t> whole( std::string_view word, std::int64_t lowest,
                                                 std::int64_t highest ) noexcept
{
    std::int64_t value = 0;
    const char *begin = word.data();
    const char *end = begin + word.size();
    const std::from_chars_result read = std::from_chars( begin, end, value );
    if( word.empty() || read.ec != std::errc{} || read.ptr != end || value < lowest ||
        value > highest ) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] int hex_digit( char digit ) noexcept
{
    if( digit >= '0' && digit <= '9' ) {
        return digit - '0';
    }
    if( digit >= 'a' && digit <= 'f' ) {
        return digit - 'a' + 10;
    }
    return -1;
}

/// The first line of a saved stack: what it is, and which form.
constexpr std::string_view saved_header = "squared screens 1";

} // namespace

/// A list shown, bound to a model's list: the widget, the list's names, and
/// what of it the cache keeps. The rows asked for (`asked`, the screen and a
/// screen each side) and their items' values stay in the cache, since the
/// model sends an item's values once, when it comes into the view.
struct Stage::ListBinding final
{
        Stage *stage = nullptr;
        gui::List *list = nullptr;
        messaging::Name rows{};
        messaging::Name count{};
        /// Which rows to ask for; sized when the list first has a height.
        messaging::ListWindow window{ 1 };
        /// The rows last asked for, kept in the cache.
        messaging::RowSpan asked{};
        /// The item kept for each row asked for, at the row modulo its size; 0: none yet.
        std::vector<std::uint32_t> kept;
        /// What a row shows of its item: `item.name`, `item.count`, ...
        std::array<messaging::Name, max_row_values> values{};
        std::uint8_t value_count = 0;
        /// The screen (`screen_owner`) or a pane's slot.
        std::uint8_t owner = 0;
        /// The reset, or the last view, found the Telegraph full: again next frame.
        bool reset_unsent = false;
        bool view_unsent = false;
        /// Said once that the cache keeps fewer rows than the list shows.
        bool told_short = false;
};

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

        /// The bound widgets of the screen and its panes, and their lists.
        /// After the scratch, so the window's size is the rest plus these.
        std::array<Binding, max_bindings> bindings{};
        std::array<ListBinding, max_lists> lists{};

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
    keep_shown(); // a window made again without a detach: what it showed is kept
    unbind_all();
    drawing_.reset();
    failed_ = false;
    // The one allocation a window makes here, besides what the skin holds:
    // the skin's pieces, the batch, the camera, the painter, the widgets,
    // and the scratch a C++ screen writes into. The widgets hold standard
    // containers, whose sizes are the library's: libc++ (Android, Termux)
    // and libstdc++ (desktop Linux) differ. The scratch is the same in both.
    static_assert( sizeof( Drawing::Scratch ) == 17480 );
    static_assert( sizeof( Binding ) == 32 );
    static_assert( sizeof( ListBinding ) == 160 );
    static_assert( sizeof( Kept ) == 148 );
    static_assert( max_lists <= 4, "Binding::list is two bits" );
    constexpr std::size_t tables = sizeof( Drawing::Scratch ) + max_bindings * 32 + max_lists * 160;
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
    // What the screen and its panes show is kept for the next window; the
    // cache may let their values go; then the panes' windows go too.
    keep_shown();
    unbind_all();
    drawing_.reset();
    for( Pane &pane : panes_ ) {
        pane.frame = nullptr;
    }
}

void Stage::clear() noexcept
{
    depth_ = 0;
    pending_count_ = 0;
    broken_ = false;
    leave_ = false;
    background_ = false;
    screens_.fill( {} );
    kept_.fill( {} );
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

void Stage::settle() noexcept
{
    // A rotation brings a size and new insets, often one call each: laid
    // out once, with both, and nothing built again.
    if( layout_due_ ) {
        lay_out();
    }
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
    layout_due_ = true;
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
        keep_shown(); // the screen is built again: its scrolls and edit with it
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
    settle(); // a finger lands where the widgets are now
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
    settle(); // a list knows its height before it asks for rows
    Showing showing{ this };
    static_cast<void>( telegraph_.drain( showing ) );
    ask_views();
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
    // What changed since the last frame is laid out again: a bound label's
    // new text, a list scrolled. Nothing, when nothing did.
    settle();
    drawing_->ui.layout( drawing_->painter );
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
            keep_shown(); // the screen beneath keeps where it was
            kept_[depth_] = {};
            screens_[depth_++] = change.shown;
            break;
        case gui::TapKind::replace:
            if( depth_ == 0 ) {
                depth_ = 1;
            }
            screens_[depth_ - 1] = change.shown;
            kept_[depth_ - 1] = {};
            break;
        case gui::TapKind::back:
            if( Pane *pane = top_pane_slot() ) {
                close_pane( *pane ); // the back button closes the top pane first
                return;
            }
            if( depth_ <= 1 ) {
                return;
            }
            kept_[depth_ - 1] = {};
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
        pane.frame = nullptr;
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
    hooks.listed = &Stage::listed;
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
    size_lists();
    lay_out();
    // A screen that comes back is where it was: laid out first, so a scroll
    // pane knows how far it scrolls; then again, for what moved.
    put_back();
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
    layout_due_ = false;
}

// --- what is kept ----------------------------------------------------------

void Stage::low_memory() noexcept
{
    // Screens beneath the top hold no widgets: nothing of theirs to let go.
    // The working sizes every table's layout keeps warm are made again by the
    // next layout.
    gui::Table::release_kept();
}

void Stage::keep_shown() noexcept
{
    gui::Ui *widgets = ui();
    if( widgets == nullptr || widgets->content() == nullptr ) {
        return; // nothing built: what was kept stays
    }
    if( depth_ != 0 ) {
        Kept &kept = kept_[depth_ - 1];
        kept = {};
        Keeping<Kept> keeping{ kept };
        walk( *widgets->content(), keeping );
    }
    for( Pane &pane : panes_ ) {
        if( pane.order != 0 && pane.frame != nullptr ) {
            pane.kept = {};
            Keeping<Kept> keeping{ pane.kept };
            walk( pane.frame->content_table(), keeping );
        }
    }
}

void Stage::put_back() noexcept
{
    gui::Ui *widgets = ui();
    if( widgets == nullptr || widgets->content() == nullptr || depth_ == 0 ) {
        return;
    }
    // What is kept is put back while it is shown; kept again when it is left.
    Putting<Kept> putting{ kept_[depth_ - 1] };
    walk( *widgets->content(), putting );
    for( Pane &pane : panes_ ) {
        if( pane.order != 0 && pane.frame != nullptr ) {
            Putting<Kept> pane_putting{ pane.kept };
            walk( pane.frame->content_table(), pane_putting );
        }
    }
}

std::string_view Stage::name_of_shown( Shown shown ) const noexcept
{
    if( shown.layout != nullptr ) {
        return shown.layout->name();
    }
    return shown.screen != nullptr ? shown.screen->name : std::string_view{};
}

std::size_t Stage::save( std::span<char> out ) noexcept
{
    if( depth_ == 0 ) {
        return 0;
    }
    keep_shown();
    // Each by its name, which must find it again: else none is kept.
    const auto found = [this]( Shown shown ) noexcept {
        const Shown again = resolve( name_of_shown( shown ) );
        return again.layout == shown.layout && again.screen == shown.screen;
    };
    LineWriter writer( out );
    writer.text( saved_header );
    writer.text( "\n" );
    const auto line = [&writer]( std::string_view kind, std::string_view name, const Kept &kept ) {
        writer.text( kind );
        writer.text( " " );
        writer.text( name );
        writer.text( " " );
        writer.number( kept.scrolls );
        for( std::size_t i = 0; i < kept.scrolls; ++i ) {
            writer.text( " " );
            writer.number( std::lround( kept.scroll[i] ) );
        }
        if( kept.field == Kept::no_field ) {
            writer.text( " - -\n" );
            return;
        }
        writer.text( " " );
        writer.number( kept.field );
        writer.text( " x" ); // the edit as hex: "x" alone is an empty one
        writer.hex( { kept.text.data(), kept.text_length } );
        writer.text( "\n" );
    };
    for( std::size_t i = 0; i < depth_; ++i ) {
        const std::string_view name = name_of_shown( screens_[i] );
        if( !found( screens_[i] ) ) {
            say( "stage: the screens are not kept: %.*s is not found by its name (a layout "
                 "file, or app.stage.add<...>())",
                 static_cast<int>( name.size() ), name.data() );
            return 0;
        }
        line( "screen", name, kept_[i] );
    }
    for( std::uint8_t order = 1; order <= pane_count_; ++order ) {
        for( const Pane &pane : panes_ ) {
            if( pane.order != order ) {
                continue;
            }
            const std::string_view name = name_of_shown( pane.shown );
            if( !found( pane.shown ) ) {
                say( "stage: the screens are not kept: the pane %.*s is not found by its name",
                     static_cast<int>( name.size() ), name.data() );
                return 0;
            }
            line( "pane", name, pane.kept );
        }
    }
    if( writer.full() ) {
        say( "stage: the screens are not kept: more than %zu bytes", out.size() );
        return 0;
    }
    return writer.written().size();
}

bool Stage::restore( std::string_view saved ) noexcept
{
    std::array<Shown, max_depth> screens{};
    std::array<Kept, max_depth> kept{};
    std::array<Shown, max_panes> panes{};
    std::array<Kept, max_panes> panes_kept{};
    std::size_t depth = 0;
    std::size_t pane_count = 0;
    std::size_t number = 0;
    const char *problem = nullptr;

    // One line: `screen <name> <scrolls> <each> <field> <edit>`, or `pane ...`.
    const auto read = [&]( std::string_view line ) noexcept -> const char * {
        const std::string_view kind = take_word( line );
        const bool screen = kind == "screen";
        if( !screen && kind != "pane" ) {
            return "not a screen or a pane";
        }
        if( screen && pane_count != 0 ) {
            return "a screen after a pane";
        }
        if( screen ? depth == max_depth : pane_count == max_panes ) {
            return screen ? "more screens than the stack holds" : "more panes than open at once";
        }
        const std::string_view name = take_word( line );
        const Shown shown = resolve( name );
        if( shown.layout == nullptr && shown.screen == nullptr ) {
            return "no layout or added screen is called that";
        }
        const gui::LayoutKind wanted = screen ? gui::LayoutKind::screen : gui::LayoutKind::pane;
        if( shown.layout != nullptr && shown.layout->kind() != wanted ) {
            return screen ? "a pane where a screen was" : "a screen where a pane was";
        }
        Kept here;
        const std::optional<std::int64_t> scrolls = whole( take_word( line ), 0, kept_scrolls );
        if( !scrolls ) {
            return "not a count of scrolls";
        }
        here.scrolls = static_cast<std::uint8_t>( *scrolls );
        for( std::size_t i = 0; i < here.scrolls; ++i ) {
            const std::optional<std::int64_t> offset = whole( take_word( line ), 0, 1 << 24 );
            if( !offset ) {
                return "not a scroll";
            }
            here.scroll[i] = static_cast<float>( *offset );
        }
        const std::string_view field = take_word( line );
        const std::string_view edit = take_word( line );
        if( field == "-" && edit == "-" ) {
            here.field = Kept::no_field;
        } else {
            const std::optional<std::int64_t> at = whole( field, 0, Kept::no_field - 1 );
            const std::size_t digits = edit.size() - 1;
            if( !at || edit.empty() || edit.front() != 'x' || digits % 2 != 0 ||
                digits / 2 > kept_text ) {
                return "not an edit";
            }
            for( std::size_t i = 0; i < digits / 2; ++i ) {
                const int high = hex_digit( edit[1 + 2 * i] );
                const int low = hex_digit( edit[2 + 2 * i] );
                if( high < 0 || low < 0 ) {
                    return "not an edit";
                }
                here.text[i] = static_cast<char>( high * 16 + low );
            }
            here.field = static_cast<std::uint8_t>( *at );
            here.text_length = static_cast<std::uint8_t>( digits / 2 );
        }
        if( !line.empty() ) {
            return "more on the line than a screen keeps";
        }
        if( screen ) {
            screens[depth] = shown;
            kept[depth++] = here;
        } else {
            panes[pane_count] = shown;
            panes_kept[pane_count++] = here;
        }
        return nullptr;
    };

    std::string_view rest = saved;
    while( problem == nullptr && !rest.empty() ) {
        const std::size_t end = rest.find( '\n' );
        const std::string_view line = rest.substr( 0, end );
        rest = end == std::string_view::npos ? std::string_view{} : rest.substr( end + 1 );
        if( number++ == 0 ) {
            problem = line == saved_header ? nullptr : "not a saved stack, or another form";
        } else {
            problem = read( line );
        }
    }
    if( problem == nullptr && depth == 0 ) {
        problem = "no screen";
    }
    if( problem != nullptr ) {
        say( "stage: the screens kept were not shown again: line %zu: %s", number, problem );
        return false;
    }

    pending_count_ = 0;
    drop_panes();
    screens_ = screens;
    kept_ = kept;
    depth_ = static_cast<std::uint8_t>( depth );
    for( std::size_t i = 0; i < pane_count; ++i ) {
        panes_[i].shown = panes[i];
        panes_[i].order = static_cast<std::uint8_t>( i + 1 );
        panes_[i].kept = panes_kept[i];
    }
    pane_count_ = static_cast<std::uint8_t>( pane_count );
    show_top();
    return true;
}

void Stage::describe( app::Report out ) noexcept
{
    keep_shown(); // as it is now
    std::array<char, 256> storage{};
    const auto said = [&out, &storage]( const auto &write ) {
        LineWriter line( storage );
        write( line );
        out( line.written() );
    };
    const auto kept_text_of = []( LineWriter &line, const Kept &kept ) {
        for( std::size_t i = 0; i < kept.scrolls; ++i ) {
            line.text( i == 0 ? "  scroll " : " " );
            line.number( std::lround( kept.scroll[i] ) );
        }
        if( kept.field != Kept::no_field ) {
            line.text( "  edit in text field " );
            line.number( kept.field );
            line.text( ", " );
            line.number( kept.text_length );
            line.text( " bytes" );
        }
    };
    said( [this]( LineWriter &line ) {
        line.text( "screens: " );
        line.number( depth_ );
        line.text( ", panes: " );
        line.number( pane_count_ );
        line.text( keep_screens_ ? "; kept in the background" : "; not kept (keep_screens off)" );
    } );
    for( std::size_t i = 0; i < depth_; ++i ) {
        said( [&]( LineWriter &line ) {
            line.text( "  " );
            line.number( static_cast<std::int64_t>( i + 1 ) );
            line.text( " " );
            line.text( name_of_shown( screens_[i] ) );
            kept_text_of( line, kept_[i] );
        } );
    }
    for( std::uint8_t order = 1; order <= pane_count_; ++order ) {
        for( const Pane &pane : panes_ ) {
            if( pane.order != order ) {
                continue;
            }
            said( [&]( LineWriter &line ) {
                line.text( "  pane " );
                line.text( name_of_shown( pane.shown ) );
                line.text( " (" );
                line.text( gui::pane_place_name( pane.place ) );
                line.text( ")" );
                kept_text_of( line, pane.kept );
            } );
        }
    }
    said( [this]( LineWriter &line ) {
        line.text( "bound: " );
        line.number( static_cast<std::int64_t>( bindings() ) );
        line.text( " values, " );
        line.number( static_cast<std::int64_t>( lists() ) );
        line.text( " lists; table sizes kept: " );
        line.number( static_cast<std::int64_t>( gui::Table::kept_count() ) );
    } );
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
        pane.kept = {};
        build_pane( pane );
        size_lists();
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
        if( below.order != 0 && below.order < pane.order && below.frame != nullptr ) {
            modal = modal || below.frame->modal();
        }
    }
    // The skin's pane frame: its background, no title bar.
    auto frame = std::make_unique<gui::PaneFrame>( "pane" );
    frame->set_modal( modal );
    gui::LayoutHooks hooks;
    hooks.tapped = &Stage::tapped_in_pane;
    hooks.bound = &Stage::bound_in_pane;
    hooks.custom = &Stage::made_in_pane;
    hooks.listed = &Stage::listed_in_pane;
    hooks.context = &pane;
    gui::Table &table = frame->content_table();
    const gui::LayoutCheck check = gui::instantiate( *layout, table, widgets->skin(), hooks );
    // A pane that did not build still opens, empty, so back closes it.
    static_cast<void>( built( *layout, check ) );
    pane.place = layout->pane().place;
    pane.frame = &widgets->show_pane( std::move( frame ) );
}

void Stage::close_pane( Pane &pane ) noexcept
{
    unbind_owner( static_cast<std::uint8_t>( &pane - panes_.data() ) );
    if( pane.frame != nullptr ) {
        if( gui::Ui *widgets = ui() ) {
            widgets->close_pane( *pane.frame ); // frees its widgets
        }
    }
    for( Pane &other : panes_ ) {
        if( other.order > pane.order ) {
            --other.order;
        }
    }
    --pane_count_;
    pane.shown = {};
    pane.frame = nullptr;
    pane.order = 0;
    pane.kept = {};
}

void Stage::drop_panes() noexcept
{
    // Their windows, if built, go with the screen's content.
    for( Pane &pane : panes_ ) {
        pane.shown = {};
        pane.frame = nullptr;
        pane.order = 0;
        pane.kept = {};
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
        if( pane.frame == nullptr ) {
            continue;
        }
        gui::PaneFrame &frame = *pane.frame;
        const gui::Size wanted = frame.size_hints( drawing_->painter, widgets.skin() ).preferred;
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
        frame.set_bounds( x, y, width, height );
        frame.invalidate_layout();
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
    auto &stage = *static_cast<Stage *>( context );
    static_cast<void>( stage.bind( screen_owner, layout, node, widget, { node.bind, 0 } ) );
}

void Stage::bound_in_pane( void *context, const gui::Layout &layout, const gui::LayoutNode &node,
                           gui::Widget &widget ) noexcept
{
    Pane &pane = *static_cast<Pane *>( context );
    const auto slot = static_cast<std::uint8_t>( &pane - pane.stage->panes_.data() );
    static_cast<void>( pane.stage->bind( slot, layout, node, widget, { node.bind, 0 } ) );
}

void Stage::bound_in_row( void *context, const gui::Layout &layout, const gui::LayoutNode &node,
                          gui::Widget &widget ) noexcept
{
    ListBinding &shown = list_of_row( context );
    Stage &stage = *shown.stage;
    const std::size_t slot = shown.list->slot_of( *static_cast<gui::Table *>( context ) );
    // A value of the row's item, at its uid (0 until the row has one), or one of its own.
    std::uint32_t uid = 0;
    if( node.row_uid && slot < shown.list->rows_built() ) {
        uid = stage.uid_at( shown, shown.list->row_at( slot ) );
    }
    const messaging::Address address{ node.bind, uid };
    const auto owner = static_cast<std::uint8_t>( slot );
    Binding *bound = stage.bind( owner, layout, node, widget, address );
    if( bound != nullptr ) {
        bound->in_row = true;
        bound->row_uid = node.row_uid;
        const auto list = static_cast<unsigned>( &shown - stage.drawing_->lists.data() );
        bound->list = static_cast<std::uint8_t>( list & 3U ); // max_lists: two bits
    }
}

Stage::Binding *Stage::bind( std::uint8_t owner, const gui::Layout &layout,
                             const gui::LayoutNode &node, gui::Widget &widget,
                             messaging::Address address ) noexcept
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
        return nullptr;
    }
    *slot = {};
    slot->address = address;
    slot->widget = &widget;
    slot->type = node.type;
    slot->value = node.value;
    slot->owner = owner;
    slot->sends_set = node.sends_set;
    // A row's item's value with no item yet (uid 0) is not an address.
    const bool addressed = !node.row_uid || address.uid != 0;
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
    if( !addressed ) {
        return slot;
    }
    // Its value stays in the cache while it is shown.
    static_cast<void>( telegraph_.bind( slot->address ) );
    if( const std::optional<messaging::CachedValue> now = telegraph_.find( slot->address ) ) {
        show( *slot, now->telegram, now->data );
    }
    return slot;
}

void Stage::unbind( Binding &binding ) noexcept
{
    if( !binding.row_uid || binding.address.uid != 0 ) {
        telegraph_.unbind( binding.address );
    }
    binding = {};
}

void Stage::unbind_all() noexcept
{
    if( drawing_ == nullptr ) {
        return;
    }
    for( Binding &binding : drawing_->bindings ) {
        if( binding.widget != nullptr ) {
            unbind( binding );
        }
    }
    bindings_full_ = false;
    drop_lists( screen_owner, true );
}

void Stage::unbind_owner( std::uint8_t owner ) noexcept
{
    if( drawing_ == nullptr ) {
        return;
    }
    for( Binding &binding : drawing_->bindings ) {
        if( binding.widget == nullptr ) {
            continue;
        }
        // A row's binding belongs to its list's owner.
        bool owned = binding.owner == owner;
        if( binding.in_row ) {
            owned = drawing_->lists[binding.list].owner == owner;
        }
        if( owned ) {
            unbind( binding );
        }
    }
    drop_lists( owner, false );
}

bool Stage::held( const Binding &binding ) noexcept
{
    switch( binding.type ) {
        case gui::NodeType::slider:
            return static_cast<const gui::Slider *>( binding.widget )->dragging();
        case gui::NodeType::text_field: {
            // An edit put back is the user's still, focused or not.
            const auto *field = static_cast<const gui::TextField *>( binding.widget );
            return field->focused() || field->edited();
        }
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

void Stage::blank( Binding &binding ) noexcept
{
    // A row whose item has not come: nothing, until it does.
    applying_ = true;
    constexpr float lowest = std::numeric_limits<float>::lowest(); // the widget's minimum
    switch( binding.type ) {
        case gui::NodeType::label:
            static_cast<gui::Label *>( binding.widget )->set_text( {} );
            break;
        case gui::NodeType::check_box:
            static_cast<gui::CheckBox *>( binding.widget )->set_checked( false );
            break;
        case gui::NodeType::slider:
            static_cast<gui::Slider *>( binding.widget )->set_value( lowest );
            break;
        case gui::NodeType::progress_bar:
            static_cast<gui::ProgressBar *>( binding.widget )->set_value( lowest );
            break;
        case gui::NodeType::text_field:
            static_cast<gui::TextField *>( binding.widget )->set_text( {} );
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
    // A list's count, or a row's item: the list and the cache follow.
    list_state( received );
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
            // A row's widget whose row has no item yet has nowhere to send.
            const bool addressed = !binding.row_uid || binding.address.uid != 0;
            return addressed ? &binding : nullptr;
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

// --- lists -----------------------------------------------------------------

std::size_t Stage::lists() const noexcept
{
    if( drawing_ == nullptr ) {
        return 0;
    }
    std::size_t count = 0;
    for( const ListBinding &shown : drawing_->lists ) {
        count += shown.list != nullptr ? 1 : 0;
    }
    return count;
}

void Stage::listed( void *context, const gui::Layout &layout, const gui::LayoutNode &node,
                    gui::List &list ) noexcept
{
    static_cast<Stage *>( context )->add_list( screen_owner, layout, node, list );
}

void Stage::listed_in_pane( void *context, const gui::Layout &layout, const gui::LayoutNode &node,
                            gui::List &list ) noexcept
{
    Pane &pane = *static_cast<Pane *>( context );
    const auto slot = static_cast<std::uint8_t>( &pane - pane.stage->panes_.data() );
    pane.stage->add_list( slot, layout, node, list );
}

Stage::ListBinding &Stage::list_of_row( void *context ) noexcept
{
    // A row's hooks have its table as their context; its list, the binding.
    const auto &row = *static_cast<const gui::Table *>( context );
    const auto *list = gui::actor_cast<gui::List>( row.parent() );
    return *static_cast<ListBinding *>( list->owner() );
}

void Stage::add_list( std::uint8_t owner, const gui::Layout &layout, const gui::LayoutNode &node,
                      gui::List &list ) noexcept
{
    const std::string_view name = name_of( layout );
    const std::string_view rows = layout.text( node.bind_text );
    ListBinding *free = nullptr;
    for( ListBinding &shown : drawing_->lists ) {
        if( shown.list != nullptr && shown.rows == node.bind ) {
            // The model keeps one view of a list: one widget shows it.
            say( "stage: %.*s: %.*s is shown already; this one stays empty",
                 static_cast<int>( name.size() ), name.data(), static_cast<int>( rows.size() ),
                 rows.data() );
            list.set_row_hooks( {}, nullptr );
            return;
        }
        if( shown.list == nullptr && free == nullptr ) {
            free = &shown;
        }
    }
    if( free == nullptr ) {
        say( "stage: %.*s: more than %zu lists; the rest stay empty",
             static_cast<int>( name.size() ), name.data(), max_lists );
        list.set_row_hooks( {}, nullptr );
        return;
    }
    ListBinding &shown = *free;
    shown = ListBinding{};
    shown.stage = this;
    shown.list = &list;
    shown.rows = node.bind;
    shown.count = messaging::Name::from_id( messaging::name_hash( ".count", node.bind.id ) );
    shown.owner = owner;
    // The values a row shows of its item: kept in the cache for every row asked for.
    const gui::Layout row = list.row();
    for( const gui::LayoutNode &inside : row.nodes() ) {
        if( !inside.row_uid || inside.bind.id == 0 ) {
            continue;
        }
        const auto end = shown.values.begin() + shown.value_count;
        if( std::find( shown.values.begin(), end, inside.bind ) != end ) {
            continue;
        }
        if( shown.value_count == max_row_values ) {
            say( "stage: %.*s: a row of %.*s shows more than %zu of its item's values",
                 static_cast<int>( name.size() ), name.data(), static_cast<int>( rows.size() ),
                 rows.data(), max_row_values );
            break;
        }
        shown.values[shown.value_count++] = inside.bind;
    }
    gui::LayoutHooks hooks;
    hooks.tapped = &Stage::tapped_in_row;
    hooks.tapped_row = &Stage::tapped_row_uid;
    hooks.bound = &Stage::bound_in_row;
    hooks.custom = &Stage::made_in_row;
    list.set_row_hooks( hooks, &shown );
    list.set_on_row( &Stage::row_shown, &shown );
    // Its count stays in the cache while it is shown.
    static_cast<void>( telegraph_.bind( { shown.count, 0 } ) );
    const std::optional<messaging::CachedValue> count = telegraph_.find( { shown.count, 0 } );
    if( count ) {
        const std::uint32_t rows_now = rows_of( count->telegram.as_integer() );
        list.set_count( rows_now );
        shown.window.set_count( rows_now );
    }
    // A new widget knows nothing: the model sends all of it again, from the count.
    const messaging::SendResult reset = telegraph_.reset_view( shown.rows, rows );
    shown.reset_unsent = reset != messaging::SendResult::sent;
}

void Stage::drop_list( ListBinding &shown ) noexcept
{
    telegraph_.unbind( { shown.count, 0 } );
    for( std::uint32_t i = 0; i < shown.asked.rows; ++i ) {
        const std::uint32_t row = shown.asked.first + i;
        telegraph_.unbind( { shown.rows, row } );
        if( shown.kept.empty() ) {
            continue; // no row asked for has kept an item yet
        }
        const std::uint32_t kept = shown.kept[row % shown.kept.size()];
        if( kept != 0 ) {
            keep_item( shown, kept, false );
        }
    }
    shown = ListBinding{};
}

void Stage::drop_lists( std::uint8_t owner, bool every ) noexcept
{
    if( drawing_ == nullptr ) {
        return;
    }
    for( ListBinding &shown : drawing_->lists ) {
        if( shown.list != nullptr && ( every || shown.owner == owner ) ) {
            drop_list( shown );
        }
    }
}

std::uint32_t Stage::uid_at( const ListBinding &shown, std::uint32_t index ) const noexcept
{
    if( index == gui::List::no_row ) {
        return 0;
    }
    const std::optional<messaging::CachedValue> row = telegraph_.find( { shown.rows, index } );
    if( !row || row->telegram.type() != messaging::ValueType::integer ) {
        return 0;
    }
    const std::int64_t uid = row->telegram.as_integer();
    return uid > 0 && uid <= UINT32_MAX ? static_cast<std::uint32_t>( uid ) : 0;
}

void Stage::keep_item( ListBinding &shown, std::uint32_t uid, bool keep ) noexcept
{
    for( std::size_t i = 0; i < shown.value_count; ++i ) {
        const messaging::Address value{ shown.values[i], uid };
        if( keep ) {
            static_cast<void>( telegraph_.bind( value ) );
        } else {
            telegraph_.unbind( value );
        }
    }
}

void Stage::row_shown( void *context, gui::List & /*list*/, std::size_t slot,
                       std::uint32_t /*index*/ ) noexcept
{
    ListBinding &shown = *static_cast<ListBinding *>( context );
    shown.stage->show_row( shown, slot );
}

void Stage::show_row( ListBinding &shown, std::size_t slot ) noexcept
{
    const auto list = static_cast<std::uint8_t>( &shown - drawing_->lists.data() );
    const std::uint32_t uid = uid_at( shown, shown.list->row_at( slot ) );
    for( Binding &binding : drawing_->bindings ) {
        const bool here = binding.widget != nullptr && binding.in_row && binding.list == list;
        if( !here || binding.owner != slot || !binding.row_uid ) {
            continue;
        }
        if( binding.address.uid == uid && uid != 0 ) {
            continue;
        }
        // Another item: its values, from the cache, or nothing until they come.
        if( binding.address.uid != 0 ) {
            telegraph_.unbind( binding.address );
        }
        binding.address.uid = uid;
        binding.stale = false;
        std::optional<messaging::CachedValue> now;
        if( uid != 0 ) {
            static_cast<void>( telegraph_.bind( binding.address ) );
            now = telegraph_.find( binding.address );
        }
        if( now ) {
            show( binding, now->telegram, now->data );
        } else {
            blank( binding );
        }
    }
}

void Stage::list_state( const messaging::Received &received ) noexcept
{
    const messaging::Address address = received.telegram.address();
    const bool integer = received.telegram.type() == messaging::ValueType::integer;
    for( ListBinding &shown : drawing_->lists ) {
        if( shown.list == nullptr || !integer ) {
            continue;
        }
        const std::int64_t value = received.telegram.as_integer();
        if( address.name == shown.count && address.uid == 0 ) {
            const std::uint32_t count = rows_of( value );
            shown.list->set_count( count );
            shown.window.set_count( count );
            return;
        }
        if( address.name != shown.rows ) {
            continue;
        }
        // Row `address.uid` shows the item `value`: the cache keeps its values
        // while the row is asked for, the new before the old let go.
        const std::uint32_t row = address.uid;
        const std::uint32_t uid = rows_of( value );
        const bool asked = row >= shown.asked.first && row - shown.asked.first < shown.asked.rows;
        if( asked && !shown.kept.empty() ) {
            std::uint32_t &kept = shown.kept[row % shown.kept.size()];
            if( kept != uid ) {
                keep_item( shown, uid, true );
                if( kept != 0 ) {
                    keep_item( shown, kept, false );
                }
                kept = uid;
            }
        }
        const std::size_t slot = shown.list->slot_showing( row );
        if( slot < shown.list->rows_built() ) {
            show_row( shown, slot );
        }
        return;
    }
}

void Stage::size_lists() noexcept
{
    if( drawing_ == nullptr ) {
        return;
    }
    // When a screen or pane is built, not in a frame: each new list's rows
    // and their items' values in at most half the cache, shared by the lists
    // shown. Its buffer is its one allocation, made here (7.8: a frame made
    // it, once the list had a height).
    const std::size_t shown_lists = lists();
    for( ListBinding &shown : drawing_->lists ) {
        if( shown.list == nullptr || !shown.kept.empty() ) {
            continue;
        }
        const std::size_t per_row = 1 + shown.value_count;
        const std::size_t room = messaging::telegraph_size::cache / 2 / shown_lists;
        const std::size_t view = messaging::telegraph_size::list_view;
        const std::size_t most = std::clamp<std::size_t>( room / per_row, 1, view );
        shown.window = messaging::ListWindow( static_cast<std::uint32_t>( most ) );
        shown.window.set_count( shown.list->count() );
        shown.kept.assign( most, 0 );
    }
}

void Stage::ask_views() noexcept
{
    if( drawing_ == nullptr ) {
        return;
    }
    for( ListBinding &shown : drawing_->lists ) {
        if( shown.list != nullptr ) {
            ask_view( shown );
        }
    }
}

void Stage::ask_view( ListBinding &shown ) noexcept
{
    const std::string_view rows_text{};
    if( shown.reset_unsent ) {
        const messaging::SendResult sent = telegraph_.reset_view( shown.rows, rows_text );
        shown.reset_unsent = sent != messaging::SendResult::sent;
        return;
    }
    const gui::List &list = *shown.list;
    const std::uint32_t screen = list.rows_on_screen();
    if( screen == 0 ) {
        return; // not laid out yet
    }
    if( shown.kept.empty() ) {
        return; // sized when it was built: size_lists()
    }
    const std::size_t most = shown.kept.size();
    if( most < screen && !shown.told_short ) {
        shown.told_short = true;
        say( "stage: a list shows %u rows; the cache keeps %zu of them: raise TELEGRAPH_CACHE",
             static_cast<unsigned>( screen ), most );
    }
    const std::optional<messaging::RowSpan> span = shown.window.visible( list.first_row(), screen );
    if( !span && !shown.view_unsent ) {
        return;
    }
    const messaging::RowSpan want = span ? *span : shown.window.window();
    const auto inside = []( messaging::RowSpan rows, std::uint32_t row ) noexcept {
        return row >= rows.first && row - rows.first < rows.rows;
    };
    // The rows no longer asked for let go, and their items' values.
    for( std::uint32_t i = 0; i < shown.asked.rows; ++i ) {
        const std::uint32_t row = shown.asked.first + i;
        if( inside( want, row ) ) {
            continue;
        }
        telegraph_.unbind( { shown.rows, row } );
        std::uint32_t &kept = shown.kept[row % shown.kept.size()];
        if( kept != 0 ) {
            keep_item( shown, kept, false );
            kept = 0;
        }
    }
    // The new ones kept, so their answers have room when they come.
    for( std::uint32_t i = 0; i < want.rows; ++i ) {
        const std::uint32_t row = want.first + i;
        if( !inside( shown.asked, row ) ) {
            static_cast<void>( telegraph_.bind( { shown.rows, row } ) );
        }
    }
    shown.asked = want;
    if( want.rows == 0 ) {
        shown.view_unsent = false;
        return;
    }
    const messaging::SendResult sent = telegraph_.view( shown.rows, rows_text, want );
    shown.view_unsent = sent != messaging::SendResult::sent;
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

void Stage::tapped_in_row( void *context, std::string_view line ) noexcept
{
    // A row's own tap, as its list's screen or pane takes it.
    const ListBinding &shown = list_of_row( context );
    Stage &stage = *shown.stage;
    if( shown.owner == screen_owner ) {
        tapped( &stage, line );
    } else {
        tapped_in_pane( &stage.panes_[shown.owner], line );
    }
}

void Stage::tapped_row_uid( void *context, std::string_view line ) noexcept
{
    const ListBinding &shown = list_of_row( context );
    Stage &stage = *shown.stage;
    const std::size_t slot = shown.list->slot_of( *static_cast<const gui::Table *>( context ) );
    std::uint32_t uid = 0;
    if( slot < shown.list->rows_built() ) {
        uid = stage.uid_at( shown, shown.list->row_at( slot ) );
    }
    if( uid == 0 ) {
        stage.say( "stage: tap %.*s: its row's item has not come yet; not sent",
                   static_cast<int>( line.size() ), line.data() );
        return;
    }
    stage.send( line, uid );
}

std::unique_ptr<gui::Widget> Stage::made_in_row( void *context, std::string_view name )
{
    return made( list_of_row( context ).stage, name );
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
            // At the root, what the back button does there: to the background.
            if( !back() ) {
                background_ = true;
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

void Stage::send( std::string_view line, std::uint32_t uid ) noexcept
{
    std::array<std::byte, 1024> storage{};
    messaging::TelegramData data( storage );
    const std::optional<messaging::Telegram> read = messaging::read_telegram( line, data );
    if( !read.has_value() ) {
        say( "stage: tap %.*s: not a telegram", static_cast<int>( line.size() ), line.data() );
        return;
    }
    // In a list's row, to the row's item.
    const messaging::Telegram telegram = uid != 0 ? read->with_uid( uid ) : *read;
    const messaging::SendResult sent = telegraph_.send( telegram, data.bytes() );
    if( sent == messaging::SendResult::full || sent == messaging::SendResult::too_large ) {
        say( "stage: tap %.*s: the Telegraph had no room; not sent",
             static_cast<int>( line.size() ), line.data() );
    }
}

} // namespace sq
