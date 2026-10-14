// The skin loader finds style classes by the last part of their libGDX names.
//
// A real uiskin.json keys its tables with full Java names
// (`com.badlogic.gdx.scenes.scene2d.ui.Label$LabelStyle`). Those belong to
// libGDX, so the loader compares only what follows the last dot, and a skin
// written for squared may say `Label$LabelStyle`. Both must work. Once the
// package part was held in the loader's code and a module rename rewrote it:
// every style in the file then counted as "not used by this GUI slice", labels
// lost their colour and nothing showed text. The tests that existed built
// their skins in code and could not see it.

#include <squared/gui/skin.hpp>
#include <squared/gui/skin_loader.hpp>
#include <squared/gui/src/color_drawable.hpp>
#include <squared/gui/src/font_resource.hpp>
#include <squared/gui/src/skin_load_issue.hpp>
#include <squared/gui/src/skin_load_report.hpp>
#include <squared/gui/src/skin_load_severity.hpp>

#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

using namespace sq::gui;

namespace
{

int g_failures = 0;

void check( bool condition, const char *what, const char *skin )
{
    if( !condition ) {
        std::printf( "  FAILED [%s]: %s\n", skin, what );
        ++g_failures;
    }
}

// The form a stock libGDX skin takes.
constexpr std::string_view k_full = R"({
  com.badlogic.gdx.graphics.g2d.BitmapFont: { default-font: { file: default.fnt } },
  com.badlogic.gdx.graphics.Color: { white: { r: 1, g: 1, b: 1, a: 1 } },
  com.badlogic.gdx.scenes.scene2d.ui.Label$LabelStyle: {
    emphasis: { font: default-font, fontColor: white }
  },
  com.badlogic.gdx.scenes.scene2d.ui.TextButton$TextButtonStyle: {
    emphasis: { up: flat, down: flat, font: default-font, fontColor: white }
  },
  com.badlogic.gdx.scenes.scene2d.ui.CheckBox$CheckBoxStyle: {
    emphasis: { checkboxOn: flat, checkboxOff: flat, font: default-font, fontColor: white }
  }
})";

// The same skin with the package left off.
constexpr std::string_view k_short = R"({
  BitmapFont: { default-font: { file: default.fnt } },
  Color: { white: { r: 1, g: 1, b: 1, a: 1 } },
  Label$LabelStyle: { emphasis: { font: default-font, fontColor: white } },
  TextButton$TextButtonStyle: {
    emphasis: { up: flat, down: flat, font: default-font, fontColor: white }
  },
  CheckBox$CheckBoxStyle: {
    emphasis: { checkboxOn: flat, checkboxOff: flat, font: default-font, fontColor: white }
  }
})";

// One class named twice, once in each form: ambiguous, so refused.
constexpr std::string_view k_twice = R"({
  Color: { white: { r: 1, g: 1, b: 1, a: 1 } },
  com.badlogic.gdx.graphics.Color: { black: { r: 0, g: 0, b: 0, a: 1 } }
})";

SkinDrawableResolver drawables()
{
    return []( std::string_view ) {
        return DrawablePtr{ std::make_shared<ColorDrawable>(
            sq::graphics::Color{ .red = 1.0F, .green = 1.0F, .blue = 1.0F, .alpha = 1.0F } ) };
    };
}

SkinFontResolver fonts()
{
    return []( std::string_view, std::string_view ) {
        return std::make_shared<FontResource>( "default.fnt" );
    };
}

void check_styles_load( std::string_view json, const char *which )
{
    Skin skin;
    SkinLoadReport report;
    const SkinDrawableResolver resolve_drawable = drawables();
    const SkinFontResolver resolve_font = fonts();
    check( load_libgdx_skin( skin, json, resolve_drawable, resolve_font, report ), "the skin loads",
           which );

    // A style that was not recognised falls back to the built-in default,
    // which has neither a font nor a colour. Finding both is the proof that
    // the class name matched.
    const LabelStyle &label = skin.label_style( "emphasis" );
    check( label.font != nullptr, "Label$LabelStyle has its font", which );
    check( label.text.has_value(), "Label$LabelStyle has its colour", which );

    const ButtonStyle &button = skin.button_style( "emphasis" );
    check( button.normal != nullptr, "TextButton$TextButtonStyle is recognised", which );
    check( button.font != nullptr, "TextButton$TextButtonStyle keeps its font", which );

    const CheckBoxStyle &check_box = skin.check_box_style( "emphasis" );
    check( check_box.font != nullptr, "CheckBox$CheckBoxStyle is recognised", which );

    for( const SkinLoadIssue &issue : report.issues ) {
        check( issue.message.find( "not used by this GUI slice" ) == std::string::npos,
               "no supported class is reported as unused", which );
    }
}

} // namespace

int main()
{
    check_styles_load( k_full, "full libGDX names" );
    check_styles_load( k_short, "short names" );

    {
        Skin skin;
        SkinLoadReport report;
        const SkinDrawableResolver resolve_drawable = drawables();
        const SkinFontResolver resolve_font = fonts();
        check( !load_libgdx_skin( skin, k_twice, resolve_drawable, resolve_font, report ),
               "one class under two names is refused", "named twice" );
    }

    if( g_failures == 0 ) {
        std::puts( "skin loader: all assertions passed" );
        return 0;
    }
    std::printf( "skin loader: %d failure(s)\n", g_failures );
    return 1;
}
