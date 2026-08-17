# Portable libGDX skin loading

`load_libgdx_skin` translates a supported libGDX skin subset into Squared's
existing primitive style records. It accepts JSON bytes and a drawable
resolver, so parsing has no dependency on SDL, Android assets, HoloDisk, or a
specific graphics backend.

The loader supports colors, bitmap-font path validation, tinted modal colors,
buttons/text buttons, text fields, check boxes, sliders, and windows. Unknown
libGDX resource classes produce warnings. A supported style with an invalid
type, unsafe font path, unknown color, or unresolved drawable is an error.

Input is treated as untrusted. The loader bounds source bytes, nesting,
resource counts, and names; rejects duplicate JSON keys and non-finite or
out-of-range colors; and requires relative contained font paths. It first
normalizes libGDX's relaxed unquoted-token syntax, then uses Squared Data's
strict owned JSON parser. The destination `Skin` is replaced only after the
entire supported subset succeeds.

```cpp
squared::gui::Skin skin;
squared::gui::SkinLoadReport report;
const bool loaded = squared::gui::load_libgdx_skin(
    skin,
    json_bytes,
    [&atlas](std::string_view name) {
        return squared::gui::resolve_atlas_drawable(atlas, name);
    },
    report
);
```

The GUI showcase uses `gdx-holo/skin/uiskin.json` and `uiskin.atlas`. Its
platform layer reads the JSON and loads the atlas, while all interpretation and
style mapping remain in Squared GUI. The atlas must outlive the resulting
skin because region drawables hold portable views of its textures.

Font rasterization and style inheritance are deliberately separate follow-up
work. Current text rendering continues through the application's `Painter`.
