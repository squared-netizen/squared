// sizeof and alignof of squared's core types, for docs/developer/memory-profile.md:
// `sqmake sizes` builds and runs it (build.lua).

import sq.app;
import sq.assets;
import sq.console;
import sq.data;
import sq.files;
import sq.graphics;
import sq.gui;
import sq.messaging;
import std;
using namespace sq;
#define P( T ) std::printf( "%-26s %4zu %4zu\n", #T, sizeof( T ), alignof( T ) )
int main()
{
    P( gui::Size );
    P( gui::Rectangle );
    P( gui::Insets );
    P( gui::SizeHints );
    P( graphics::Color );
    P( gui::PointerEvent );
    P( gui::TooltipConfig );
    P( gui::FontResource );
    P( gui::ColorDrawable );
    P( gui::RegionDrawable );
    P( gui::NinePatchDrawable );
    P( gui::DrawablePtr );
    P( gui::FontPtr );
    P( gui::ButtonStyle );
    P( gui::LabelStyle );
    P( gui::WindowStyle );
    P( gui::Skin );
    P( gui::Actor );
    P( gui::Group );
    P( gui::Widget );
    P( gui::Label );
    P( gui::Button );
    P( gui::ToggleButton );
    P( gui::CheckBox );
    P( gui::TextField );
    P( gui::Slider );
    P( gui::ProgressBar );
    P( gui::Panel );
    P( gui::Cell );
    P( gui::Table );
    P( gui::LinearLayout );
    P( gui::ScrollPane );
    P( gui::List );
    P( gui::PaneFrame );
    P( gui::Ui );
    P( gui::ButtonGroup );
    P( gui::InputEvent );
    P( gui::SkinLoadIssue );
    P( graphics::TextureRegion );
    P( graphics::BitmapGlyph );
    P( graphics::GlyphPlacement );
    P( graphics::BitmapFontError );
    P( app::Event );
    P( files::FileHandle );
    P( files::FileError );
    P( data::JsonValue );
    P( data::JsonError );
    P( assets::AssetError );
    P( messaging::Name );
    P( messaging::Address );
    P( messaging::NameTable );
    P( messaging::Telegram );
    P( console::Scrollback );
    P( console::InputLine );
    P( console::Commands );
    P( console::Console );
    P( console::Quad );
    P( console::Frame );
    P( console::Renderer );
    P( std::string );
    P( std::function<void()> );
    return 0;
}
