# Portable libGDX skin loading

`load_libgdx_skin` translates a supported libGDX skin subset into Squared's
portable resources and typed style records. It accepts JSON bytes plus
drawable and font resolvers, so parsing has no dependency on SDL, Android
assets, HoloDisk, or a specific graphics backend.

The loader supports colors, bitmap fonts, tinted modal colors, labels,
buttons/text buttons, text fields, check boxes, sliders, progress bars, and
windows. ScrollPane styles supply vertical and horizontal `ScrollBar` thumbs;
List styles supply `ListView` background, selection, font, and colors. Unknown
libGDX resource classes produce warnings. A supported style with an invalid
type, unsafe font path, unresolved resource, or invalid inheritance graph is
an error.

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
    [&assets](std::string_view name, std::string_view descriptor) {
        return assets.resolve_gui_font(name, descriptor);
    },
    report
);
```

The compatibility overload omits the font resolver. It still imports safe
font declarations as descriptor-only `FontResource` objects, allowing an
existing painter to use its default font while applications migrate.

Within each supported style class, `parent` or `extends` copies a named style
before applying the child's declared fields. Resolution is order-independent
and same-class only. Missing parents, cycles, and specifying both keywords are
errors; the destination skin remains unchanged.

The GUI showcase uses `gdx-holo/skin/uiskin.json` and `uiskin.atlas`. Its
platform layer reads the JSON and loads the atlas, while all interpretation and
style mapping remain in Squared GUI. The atlas must outlive the resulting
skin because region drawables hold portable views of its textures.

Font page textures and atlas textures remain application-owned and must outlive
the resulting skin. `FontResource` owns portable metrics and region values;
the painter decides how those glyph regions are submitted to the backend.
