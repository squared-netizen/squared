# dialogs

Dialogs any app shows over its screen. `import sq.dialogs;`, and link
`squared.dialogs` (it is not part of `import squared;`). Design:
docs/design/preferences.md in the squared repository, "The file dialog".

## The file dialog

Where a file comes from or goes: a modal pane in the middle of the screen,
the same in every app.

```cpp
import sq.dialogs;

void exported( void *context, std::string_view path ) noexcept
{
    if( path.empty() ) {
        return; // Cancel
    }
    // write the file at `path`
}

sq::FileDialogOptions options;
options.mode = sq::FileDialogMode::save;   // open, save or folder
options.title = "Export";
options.name = "notes.sq";                 // to start with; the person can change it
options.extensions = { ".sq" };            // what is shown, and the ending a saved name gets
options.chosen = &exported;
options.context = this;
sq::FileDialog::show( app.stage, std::move( options ) );
```

| Mode | Shows | Chooses |
|---|---|---|
| `open` | folders, and the files with one of `extensions` (all, when none) | a file tapped, then Open |
| `save` | the same, and a Name field; says when the name would replace a file | the folder and the name, with the first ending when it has none of them |
| `folder` | folders only | the folder shown (Use this folder) |

- **Places**: a row of buttons, `options.places`, or the app's own
  (`FileDialog::places_of`): App (its files), Shared (shared storage),
  Given (the files it was given), each where the platform has one, and Top.
- **Where it starts**: `options.folder`, else the first place.
- **The list**: folders first, then files, each by name in any case. Names
  that start with a dot are left out unless `options.hidden`. A folder that
  cannot be read says why; past 2,000 entries the rest are counted, not shown.
- **What the app is told**: `chosen( context, path )` once, as the dialog
  closes, on the UI thread: the whole path, or empty for Cancel. A callback
  may show another dialog.
- **One at a time**: `show` is false when one is showing.

It reads real paths with `std::filesystem` (the app's own folders and shared
storage, as the platform names them), when a folder is entered; it does not
watch one.

Its state is a `FileDialog` (`go`, `enter`, `up`, `set_name`, `choice`,
`replaces`, `finish`), which the pane shows and a test can drive without a
window (`dialogs/test/file_dialog_test.cpp`).

## In Settings

The Settings screen's Export... and Import... use it. What they do with the
file chosen is, until the app says otherwise with
`sq::Settings::set_transfer` (`sq.settings`), the preferences' own:
`Preferences::export_to` packs them (settings and files) into one cartridge,
and `Preferences::import_from` takes one back, refusing another app's or
another kind, replacing them whole (`ImportMode::merge` lays it over them
instead). Both go through `sq.drive`, vendored in `third_party/sqdrive` with
the sqcart it builds with (`tools/vendor_sqdrive.sh`).
