#!/usr/bin/env bash
# Print sizeof/alignof for the core types, for docs/developer/memory-profile.md.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
: "${CXX:=g++}"
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

includes=""
for candidate in */include; do
  if [ -d "$candidate" ]; then
    includes="$includes -I$root/$candidate"
  fi
done
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
cat > "$work/sizes.cpp" <<'EOF'
#include <squared/gui/gui.hpp>
#include <squared/scene2d/scene2d.hpp>
#include <squared/graphics2d/graphics2d.hpp>
#include <cstdio>
using namespace squared;
#define P(T) std::printf("%-26s %4zu %4zu\n", #T, sizeof(T), alignof(T));
int main() {
  P(gui::Size) P(gui::Rectangle) P(gui::Insets) P(gui::SizeHints)
  P(graphics::Color) P(gui::PointerEvent) P(gui::TooltipConfig)
  P(gui::FontResource) P(gui::ColorDrawable) P(gui::RegionDrawable)
  P(gui::NinePatchDrawable) P(gui::DrawablePtr) P(gui::FontPtr)
  P(gui::ButtonStyle) P(gui::LabelStyle) P(gui::WindowStyle) P(gui::Skin)
  P(scene2d::Actor) P(scene2d::Group) P(gui::Widget) P(gui::Label)
  P(gui::Button) P(gui::ToggleButton) P(gui::CheckBox) P(gui::TextField)
  P(gui::Slider) P(gui::ProgressBar) P(gui::Panel) P(gui::Cell)
  P(gui::Table) P(gui::LinearLayout) P(gui::ScrollPane) P(gui::Window)
  P(gui::Dialog) P(gui::Ui) P(gui::ButtonGroup)
  P(graphics2d::TextureRegion) P(graphics2d::BitmapGlyph)
  P(graphics2d::GlyphPlacement) P(std::string) P(std::function<void()>)
  return 0;
}
EOF
# shellcheck disable=SC2086
"$CXX" -std=c++20 -O2 $includes $SQUARED_EXTERNAL_INCLUDES \
  "$work/sizes.cpp" -o "$work/sizes"
"$work/sizes"
