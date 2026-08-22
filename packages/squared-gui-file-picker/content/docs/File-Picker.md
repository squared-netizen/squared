# Squared GUI File Picker

This optional module composes the ordinary Squared GUI hierarchy into a modal,
resizable file picker. It requires Squared GUI `0.6.0-dev.17` and Squared
HoloDisk `0.6.0-dev.3`; selecting the module through `squared-pg` installs both
coordinates transitively.

`FilePicker` borrows an application-owned `HoloDrive` and `AssetManager`.
Directory enumeration uses `HoloDrive::list`; accepted files can be loaded with
`load_selected<T>()`. The helpers register a bounded text asset loader and load
the packaged gdx-holo JSON through `AssetManager` before the existing
transactional libGDX skin importer commits it.

```cpp
using namespace squared;

gui::FilePickerOptions options;
options.initial_path = "/documents";

auto picker = std::make_unique<gui::FilePicker>(
    drive,
    assets,
    options,
    [](const gui::FilePickerResult& result) {
        if (result.accepted) open_document(result.path);
    }
);
ui.show_window(std::move(picker));
```

The application must mount its packaged Android assets into the drive namespace
before using `gdx_holo_skin_json_path`. Drawable and bitmap-font creation remain
injected so this package has no SDL, Android, OpenGL, or platform filesystem
dependency. There is no Lua binding in this release.
