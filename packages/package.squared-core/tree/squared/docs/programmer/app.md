# app

The platform-neutral boundary. Developer code implements `Application`; the
platform adapter owns process setup, the event pump and the soft keyboard.
Nothing above this layer names SDL or Android.

Developer counterpart: [../developer/app.md](../developer/app.md)

| Type | Header | Purpose |
|---|---|---|
| `KeyModifier` | `squared/app/key_modifier.hpp` | Platform-neutral modifier keys carried by keyboard events |
| `KeyModifiers` | `squared/app/key_modifiers.hpp` | Compact value object containing zero or more modifier keys |
| `Event` | `squared/app/event.hpp` | Platform-neutral application event |
| `TextInputPurpose` | `squared/app/text_input_purpose.hpp` | Desired soft-keyboard content mode |
| `TextInputArea` | `squared/app/text_input_area.hpp` | Soft-keyboard target rectangle in stage/logical coordinates |
| `TextInputRequest` | `squared/app/text_input_request.hpp` | Request sent when platform text input starts |
| `TextInputService` | `squared/app/text_input_service.hpp` | Portable application-to-platform soft-keyboard boundary |
| `Application` | `squared/app/application.hpp` | Developer-owned application behind a platform-neutral lifecycle |

## Aggregate headers

`squared/app/application.hpp` is both the `Application` interface and
the module's aggregate header: it pulls in `Event` and the text-input types,
because implementing `Application` needs all of them.
`squared/app/text_input.hpp` aggregates the four text-input types.

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
`surface_destroyed` and `set_text_input_service` have empty defaults, so a
desktop-only application can ignore them &mdash; but on Android
`surface_destroyed` and `surface_created` are where GPU resources are
invalidated and restored, and ignoring them is a bug that only shows on device.

## Feeding the GUI

`sq::gui::Ui::event()` takes an `Event` directly, so a typical
`handle_event` is one line:

```cpp
void MyGame::handle_event(const sq::app::Event& event)
{
    ui_.event(event);
}
```

Pass the `TextInputService*` you receive in `set_text_input_service` to the
`Ui` so focused text fields raise the soft keyboard.
