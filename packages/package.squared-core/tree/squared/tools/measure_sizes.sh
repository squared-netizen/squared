#!/usr/bin/env bash
# Print sizeof/alignof for the core types, for docs/developer/memory-profile.md.
# Imports the modules: build first (make), with the same CXX.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
: "${CXX:=clang++}"
: "${BUILD:=release}"
: "${SQUARED_EXTERNAL_INCLUDES:=}"

# Pick up local.mk the way build/common.mk does, so this script and the build
# see the same external modules.
if [ -z "$SQUARED_EXTERNAL_INCLUDES" ] && [ -f local.mk ]; then
  SQUARED_EXTERNAL_INCLUDES="$(
    sed -n 's/^[[:space:]]*SQUARED_EXTERNAL_INCLUDES[[:space:]]*[:?]\{0,1\}=[[:space:]]*//p' \
      local.mk | tr '\n' ' ' | sed 's/\\//g'
  )"
  # local.mk is make syntax, so it may contain $(PREFIX). Rewrite make's
  # $(NAME) to shell's ${NAME} before expanding: passing $(...) through eval
  # makes the shell try to run the name as a command.
  SQUARED_EXTERNAL_INCLUDES="$(
    printf '%s' "$SQUARED_EXTERNAL_INCLUDES" \
      | sed 's/\$(\([A-Za-z_][A-Za-z_0-9]*\))/${\1}/g'
  )"
  eval "SQUARED_EXTERNAL_INCLUDES=\"$SQUARED_EXTERNAL_INCLUDES\""
fi

includes="-I$(dirname "$root")"
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
cat > "$work/sizes.cpp" <<'EOF'
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
#define P(T) std::printf("%-26s %4zu %4zu\n", #T, sizeof(T), alignof(T));
int main() {
  P(gui::Size) P(gui::Rectangle) P(gui::Insets) P(gui::SizeHints)
  P(graphics::Color) P(gui::PointerEvent) P(gui::TooltipConfig)
  P(gui::FontResource) P(gui::ColorDrawable) P(gui::RegionDrawable)
  P(gui::NinePatchDrawable) P(gui::DrawablePtr) P(gui::FontPtr)
  P(gui::ButtonStyle) P(gui::LabelStyle) P(gui::WindowStyle) P(gui::Skin)
  P(gui::Actor) P(gui::Group) P(gui::Widget) P(gui::Label)
  P(gui::Button) P(gui::ToggleButton) P(gui::CheckBox) P(gui::TextField)
  P(gui::Slider) P(gui::ProgressBar) P(gui::Panel) P(gui::Cell)
  P(gui::Table) P(gui::LinearLayout) P(gui::ScrollPane) P(gui::List) P(gui::PaneFrame)
  P(gui::Ui) P(gui::ButtonGroup)
  P(gui::InputEvent) P(gui::SkinLoadIssue)
  P(graphics::TextureRegion) P(graphics::BitmapGlyph)
  P(graphics::GlyphPlacement) P(graphics::BitmapFontError)
  P(app::Event) P(files::FileHandle) P(files::FileError)
  P(data::JsonValue) P(data::JsonError) P(assets::AssetError)
  P(messaging::Name) P(messaging::Address) P(messaging::NameTable) P(messaging::Telegram) P(console::Scrollback) P(console::InputLine) P(console::Commands) P(console::Console)
  P(console::Quad) P(console::Frame) P(console::Renderer)
  P(std::string) P(std::function<void()>)
  return 0;
}
EOF
lib="$root/build/$BUILD/lib"
[ -f "$lib/libsquared_std.a" ] || { echo "measure_sizes: build first (make), the modules' BMIs are read" >&2; exit 2; }
# shellcheck disable=SC2086
"$CXX" -std=c++23 -stdlib=libc++ -pthread -D__BIONIC_CTYPE_INLINE=inline -O2 $includes $SQUARED_EXTERNAL_INCLUDES \
  -fprebuilt-module-path="$root/build/$BUILD/modules" "$work/sizes.cpp" \
  -Wl,--start-group "$lib"/libsquared_*.a -Wl,--end-group -o "$work/sizes"
"$work/sizes"
