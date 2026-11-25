# Preferences and the Settings screen

Status: proposed (2026-11-25).

Every squared app has settings, and most of them are the same few: the
skin, the font (one of the skin's bitmap fonts, or a TTF/OTF file), its
size, the colours, whether the keyboard comes up at the start. squared
gives them once: **a preferences cartridge** that holds everything an app
keeps, and **a Settings screen** that shows the invariants and whatever the
app adds, with import and export built in. sqterm and sqIDE are the first
two users; sqIDE's Settings (docs/design.md there) is this screen.

## 1. The preferences cartridge

An app's preferences are one squared cartridge (sqcart 3.0), as every
workspace is one, and held the same way: **exploded** while in use, packed
only to travel.

```
<app Local storage>/preferences/
  SQ-INF/manifest.json     kind "preferences", id "preferences.<app id>"
  tree/
    settings.json          the typed settings (below)
    ...                    anything else the app keeps: any file type
```

- **kind** `preferences`; **id** `preferences.` and the app's id
  (`preferences.dev.squared.sqide`); **version** the schema's (below), so
  an import from an older app can be migrated.
- **settings.json**: one JSON object (sq.data), keys dotted by section:
  `ui.skin`, `ui.font.file`, `ui.font.size`, `ui.theme.0` ... `ui.theme.19`,
  `input.keyboard_on_start`, and the app's own (`terminal.scrollback`). Only
  what differs from the default is written.
- **Any other file**: a TTF the person added (`fonts/`), a skin of their own
  (`skins/NAME.json` with its atlas), a key, a history, an app's own
  `theme.json`. The cartridge does not know what they are; the app does.

### `sq.preferences`

A module of squared's, on the UI thread (the model reads a copy it is
sent, as a part's argument or a telegram; it never opens the files).

| | |
|---|---|
| `Preferences::open(files, app_id)` | the cartridge, made with the defaults when it is not there |
| `get<T>(key, fallback)`, `set(key, value)`, `reset(key)` | a typed setting; `set` writes settings.json at once (whole, atomically: a new file renamed over the old) |
| `path(name)`, `read(name)`, `write(name, bytes)`, `remove(name)` | a file of the app's, under `tree/` |
| `generation()` | counts changes, as `term.theme_generation` does: what draws from a setting reads it again when it moves |
| `export_to(file)` | the cartridge packed into one `.sq` (sq.drive's pack) |
| `import_from(file, Replace or Merge)` | a `.sq` of kind preferences: checked, exploded into a staging folder beside the cartridge, then swapped in (Replace) or its files and keys laid over (Merge) |

Import refuses a cartridge of another kind; one of another app's id is
offered with a warning (a theme moves between sqterm and sqIDE). A newer
schema than the app knows is refused; an older one is migrated by the app's
own step (`on_migrate(from, settings)`).

squared gets the cartridge I/O by vendoring **sqdrive** (`third_party/
sqdrive`, as square does): sqcart is the format, sqdrive the I/O, and
squared adds only what a preferences cartridge is. The module is not part
of `import squared;` for an app that wants none of it.

## 2. The Settings screen

`sq::SettingsScreen`, in squared (sq.stage, drawn with sq.gui): one screen,
made from **sections**, each a list of **entries**, each bound to one key
of the preferences. A change is written at once and published; there is no
Save button.

### What every app gets

| Section | Entries |
|---|---|
| Appearance | Skin: the app's bundled skins and the ones in `tree/skins/` · UI scale |
| Text | Font: the skin's bitmap fonts, or a TTF/OTF file (the bundled ones and `tree/fonts/`, or Add font..., which copies one in) · Size |
| Colours | the theme's palette, when the app declares one (16 + 4 for a terminal) |
| Input | Keyboard on start |
| Preferences | Export..., Import..., Reset all |

An app turns any of these off, and adds its own.

### An app's own sections

Declared as data, in code or as JSON next to the app's layouts (sqgui's
editor can show it later):

```json
{"section": "Terminal", "entries": [
  {"key": "terminal.scrollback", "type": "number", "label": "Scrollback", "min": 100, "max": 100000, "default": 5000},
  {"key": "terminal.bell", "type": "choice", "label": "Bell", "choices": ["off", "flash", "vibrate"], "default": "flash"},
  {"key": "terminal.shell", "type": "file", "label": "Shell", "kinds": ["program"], "default": "zsh"}
]}
```

Entry types: `bool`, `number` (a range: a slider, or a field), `text`,
`choice`, `colour`, `file` (one of a folder's, or added), `action` (a
button that calls the app). A `file` entry for a font, a skin or a sound
copies what is added into the cartridge, so an export carries it.

### Export and import

- **Export...** writes `<app id>-preferences-<date>.sq` to shared storage
  (FileType::External: the folder sqterm already uses for packages) and
  says where.
- **Import...** lists the `.sq` files of kind preferences there (no system
  picker needed: they are the app's own folder's), then Replace or Merge.
  After an import, every setting is applied again (the generation moves).

## 3. sqterm and sqIDE

- What they keep today moves into the cartridge on first start:
  `~/.config/sqterm/theme.json` -> `tree/theme.json` (and its colours to
  `ui.theme.*`), `files/terminal.font` -> `ui.font.*`. The old files stay
  read for one release, then go.
- sqterm gets the screen as its first screen besides the terminal (its
  drawer: Settings); sqIDE's left drawer: Settings, on every screen.
- Their own sections: Terminal (scrollback, bell, the shell), and sqIDE's
  Workspaces (where they are kept; the default template).

## 4. Phases

1. `sq.preferences`: the cartridge, typed settings, files, export and
   import (sqdrive vendored); tests on the host, round trip through a `.sq`.
2. `sq::SettingsScreen`: sections and entries, the invariants, data-declared
   sections; the keyboard as the wizard's fields have it.
3. sqterm and sqIDE: their settings moved in, the screen in their drawers.
4. sqgui: sections shown and edited in the GUI editor.

## Open questions

- UI scale: the Stage's, applied live, or at the next start?
- A skin of the person's own: only the JSON and its atlas, or also the
  bitmap fonts it names?
- Export with a date in the name, or one file overwritten each time?
