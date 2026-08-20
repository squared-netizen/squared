# Squared GUI File Picker programmer guide

[Developer counterpart](../../developer/squared-gui-file-picker/README.md)

`squared_gui_file_picker` is the optional bridge between Squared GUI and
HoloDisk. Its package manifest requires GUI `0.6.0-dev.15` and HoloDisk
`0.6.0-dev.3`, so adding the file-picker coordinate with `squared-pg` brings in
both dependencies.

## Use

Create and mount the application-owned `HoloDrive`, bind an `AssetManager`, and
show a `FilePicker` through the existing `Ui`:

```cpp
auto picker = std::make_unique<squared::gui::FilePicker>(
    drive,
    assets,
    squared::gui::FilePickerOptions{.initial_path = "/documents"},
    [](const squared::gui::FilePickerResult& result) {
        if (result.accepted) open_document(result.path);
    }
);
ui.show_window(std::move(picker));
```

The drive and manager are borrowed and must outlive the picker. Calls stay on
their owning thread. Navigation is transactional: a failed listing preserves
the current path and exposes a diagnostic through `last_error()`.

`files`, `directories`, and `files_and_directories` modes control acceptance.
The selected file may be loaded through a registered typed loader with
`load_selected<T>()`.

## gdx-holo

GUI dev.15 already packages the selected gdx-holo JSON, atlas, texture, and
bitmap font. Mount generated-project assets at `/assets`, register or reuse the
file-picker text loader, then call `load_file_picker_holo_skin`. Drawable and
font resolvers remain application-injected because texture construction belongs
to the selected graphics backend. The Skin is unchanged when loading fails.

The picker uses `default` Window, Button, TextField, and Label styles, so its
ordinary child widgets inherit gdx-holo without a file-picker-specific renderer.
There is no Lua binding in this release.
