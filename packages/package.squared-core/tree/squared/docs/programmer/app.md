# app

The platform-neutral boundary. The platform adapter owns process setup, the
event pump and the soft keyboard, and drives an `Application`. Nothing above
this layer names SDL or Android.

**An app does not implement `Application` any more** (Sprint 7.1): `sq::App`
does, and an app writes `squared_main` and its model ([stage.md](stage.md)).
This page is for a platform layer, and for a program that wants the bare
boundary without the App.

Developer counterpart: [../developer/app.md](../developer/app.md)

| Type | Header | Purpose |
|---|---|---|
| `Key` | `squared/app/src/key.hpp` | Portable key names; the one key type in squared, shared with the GUI |
| `KeyModifiers` | `squared/app/src/key_modifiers.hpp` | The modifier keys held: four `bool` fields in one byte |
| `Navigation` | `squared/app/src/navigation.hpp` | Semantic focus and navigation action from any supported device |
| `Event` | `squared/app/event.hpp` | Platform-neutral application event |
| `TextInputPurpose` | `squared/app/src/text_input_purpose.hpp` | Desired soft-keyboard content mode |
| `TextInputArea` | `squared/app/src/text_input_area.hpp` | Soft-keyboard target rectangle in stage/logical coordinates |
| `TextInputRequest` | `squared/app/src/text_input_request.hpp` | Request sent when platform text input starts |
| `TextInputService` | `squared/app/src/text_input_service.hpp` | Portable application-to-platform soft-keyboard boundary |
| `Report` | `squared/app/src/report.hpp` | Where an application says what went wrong: a function pointer and its context, given by the platform |
| `Application` | `squared/app/application.hpp` | The application behind a platform-neutral lifecycle; `sq::App` is the one an app uses |

## Aggregate headers

`squared/app/application.hpp` is both the `Application` interface and
the module's aggregate header: it pulls in `Event` and the text-input types,
because implementing `Application` needs all of them.
`squared/app/src/text_input.hpp` aggregates the four text-input types.

## Keys, modifiers and navigation

There is one of each in squared, and the GUI uses the same ones: what the
platform puts in an `Event` is exactly what a widget's `key_down` receives.

```cpp
#include <squared/app/event.hpp>

if (event.type == sq::app::Event::Type::KeyDown &&
    event.key == sq::app::Key::enter && event.modifiers.control) {
    submit();
}

sq::app::KeyModifiers held{.shift = true};   // build one with designated initializers
if (held.any()) { /* ... */ }
```

| Type | Values |
|---|---|
| `Key` | `unknown`, `left`, `right`, `up`, `down`, `home`, `end`, `backspace`, `delete_key`, `enter`, `space`, `tab`, `escape`. A key squared doesn't name arrives as `unknown`, with the platform's code in `Event::input_device_id` |
| `KeyModifiers` | the fields `shift`, `control`, `alt`, `meta`; `any()`; `==` |
| `Navigation` | `unknown`, `left`, `right`, `up`, `down`, `next`, `previous`, `activate`, `cancel` |

## Implementing an Application

```cpp
#include <squared/app/application.hpp>

class MyGame final : public sq::app::Application {
public:
    bool create(sq::graphics::Context& graphics) override;
    void handle_event(const sq::app::Event& event) override;
    void update(std::chrono::nanoseconds delta) override;
    void render(sq::graphics::Context& graphics) override;
    void dispose() override;
    [[nodiscard]] bool quit_requested() const noexcept override
    {
        return quit_;
    }

private:
    bool quit_{false};
};
```

`create`, `handle_event`, `update`, `render`, `dispose` and `quit_requested`
are required. `pause`, `resume`, `resize`, `surface_created`,
`surface_destroyed`, `set_text_input_service`, `set_report`, `foreground`,
`background`, `low_memory`, `set_saved_state`, `save_state`,
`take_background_request` and `hides_bars` have empty defaults, so a desktop-only
application can ignore them &mdash; but on Android `surface_destroyed` and
`surface_created` are where GPU resources are invalidated and restored, and
ignoring them is a bug that only shows on device.

| Call | When the platform makes it |
|---|---|
| `set_report(report)` | before `create()`: `report(line)` says something went wrong (Android: the system log, and the Console when built in) |
| `foreground()` | the app is visible again (Android's start); not before the first `create()` |
| `background()` | the app is no longer visible (Android's stop): it may never come back, so state is made safe here |
| `low_memory()` | the system is short of memory |
| `set_saved_state(bytes)` | before each `create()`: what Android kept for the activity (its saved instance state), valid until `create()` returns. Empty for a new launch: after a swipe from the recent apps, a finish, a force stop or a reboot; Android keeps it only when it ended the process itself or made the activity again |
| `save_state(out)` | Android asks what to keep with the activity: write at most `Application::saved_state_bytes` (8 KB) into `out`, and return how many; 0 for nothing |
| `take_background_request()` | each loop turn: true, once, when the application asked to go to the background (`sq::App`: back at its root screen); the platform moves its task back, as Home does |
| `hides_bars()` | each frame: whether the status and navigation bars should be hidden (immersive); the platform hides or shows them when the answer changes, and again after each focus |

## Feeding the GUI

`sq::App` is the `Application` an app uses: it hands each event to its
Stage, which routes it to the widgets, and gives the `TextInputService*` it
receives to them, so a focused text field raises the soft keyboard. The
widgets' host it uses (`gui::Ui`) is internal since 7.7.

## The safe area

Android draws the application edge to edge, so the status bar, the navigation
bar and a camera cut-out sit on top of it. The platform layer measures how far
they reach and tells you:

```cpp
void MyGame::safe_area_changed(const sq::app::SafeArea& area)
{
    // pixels: area.left, area.top, area.right, area.bottom
    // (sq::App passes it to its Stage, which keeps screens and panes inside it)
}
```

| Type | Header | Purpose |
|---|---|---|
| `SafeArea` | `squared/app/safe_area.hpp` | Four 16-bit pixel counts: how far something covers each edge of the drawable |

`safe_area_changed()` is optional (the default ignores it) and is called after
`create()` once the bars are known, then whenever they change &mdash; rotation,
the bars hidden or shown (they are hidden by default: `hides_bars()`), the
keyboard. On Android it is measured on those events only, never on a timer
(since 7.8; it was twice a second). The first call can come a fraction of a second
after the first frame, because Android has not laid the window out before
then. A platform with nothing covering the window never calls it.

Values are in drawable pixels, like `resize()`. Convert to your logical unit
the same way you convert pointer positions.

Lua: not bound.

## The runtime

`create()` receives a `sq::app::Runtime` &mdash; the three services only the
platform layer can build:

```cpp
bool MyGame::create(sq::app::Runtime& runtime)
{
    graphics_ = &runtime.graphics;
    files_    = &runtime.files;
    assets_   = &runtime.assets;

    auto atlas = runtime.files.internal("skins/default/skin/uiskin.atlas");
    return atlas.exists();
}
```

| Member | What it is |
|---|---|
| `graphics` | the rendering context; survives surface loss |
| `files` | storage: `internal` is the read-only APK bundle, `local` is private writable storage, `external` is shared |
| `assets` | the typed asset cache, reading through `files` |

Keep references to what you use. Every member refers to an object that lives
for the whole process.

Only `create()` gets the runtime. `render()` still takes the context alone,
because a frame should not be reaching into the file system or the asset cache
sixty times a second.

### On Android, the storage names cross

| squared | Android calls it | |
|---|---|---|
| `internal` | the APK's `assets/` | read-only, ships with the app |
| `local` | **internal storage** | writable, private, no permission, gone on uninstall |
| `external` | external storage | may be unmounted, so may be empty |

Save games, settings and caches go in `local`. It needs no permission and is
removed with the app.
