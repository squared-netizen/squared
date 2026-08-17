#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_net.h>
#include <SDL_ttf.h>

#include <squared_gui_showcase/application.hpp>
#include <squared/application/application.hpp>
#include <squared/application/event.hpp>
#include <squared/application/text_input.hpp>
#include <squared/graphics/context.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>

namespace {

constexpr int logical_width = 960;
constexpr int logical_height = 540;

class PlatformLibraries final {
public:
    [[nodiscard]] bool initialize() noexcept
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO |
                     SDL_INIT_GAMECONTROLLER) != 0) return false;
        sdl_ready_ = true;
        ttf_ready_ = TTF_Init() == 0;
        const int image_flags = IMG_INIT_PNG | IMG_INIT_JPG;
        image_ready_ = (IMG_Init(image_flags) & image_flags) == image_flags;
        const int mixer_flags = MIX_INIT_OGG | MIX_INIT_MP3;
        mixer_ready_ = (Mix_Init(mixer_flags) & mixer_flags) == mixer_flags;
        net_ready_ = SDLNet_Init() == 0;
        return true;
    }

    ~PlatformLibraries()
    {
        if (net_ready_) SDLNet_Quit();
        if (mixer_ready_) Mix_Quit();
        if (image_ready_) IMG_Quit();
        if (ttf_ready_) TTF_Quit();
        if (sdl_ready_) SDL_Quit();
    }

private:
    bool sdl_ready_{false};
    bool ttf_ready_{false};
    bool image_ready_{false};
    bool mixer_ready_{false};
    bool net_ready_{false};
};

class SdlTextInputService final
    : public squared::application::TextInputService {
public:
    void start(const squared::application::TextInputRequest& request) override
    {
        update_area(request.area);
        SDL_StartTextInput();
        active_ = true;
    }

    void update_area(
        const squared::application::TextInputArea& area
    ) override
    {
        int window_width = logical_width;
        int window_height = logical_height;
        if (SDL_Window* window = SDL_GetKeyboardFocus()) {
            SDL_GetWindowSize(window, &window_width, &window_height);
        }
        const float scale_x = static_cast<float>(window_width) / logical_width;
        const float scale_y = static_cast<float>(window_height) / logical_height;
        SDL_Rect rectangle{
            static_cast<int>(area.x * scale_x),
            static_cast<int>(area.y * scale_y),
            std::max(1, static_cast<int>(area.width * scale_x)),
            std::max(1, static_cast<int>(area.height * scale_y))
        };
        SDL_SetTextInputRect(&rectangle);
    }

    void stop() override
    {
        SDL_StopTextInput();
        active_ = false;
    }

    [[nodiscard]] bool active() const noexcept override { return active_; }

private:
    bool active_{false};
};

class SdlControllers final {
public:
    SdlControllers()
    {
        for (int index = 0; index < SDL_NumJoysticks(); ++index) open(index);
    }

    ~SdlControllers()
    {
        for (const auto& [id, controller] : controllers_) {
            static_cast<void>(id);
            SDL_GameControllerClose(controller);
        }
    }

    void handle(const SDL_Event& event)
    {
        if (event.type == SDL_CONTROLLERDEVICEADDED) {
            open(event.cdevice.which);
        } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
            const auto found = controllers_.find(event.cdevice.which);
            if (found != controllers_.end()) {
                SDL_GameControllerClose(found->second);
                controllers_.erase(found);
            }
        }
    }

private:
    void open(int device_index)
    {
        if (!SDL_IsGameController(device_index)) return;
        SDL_GameController* controller = SDL_GameControllerOpen(device_index);
        if (!controller) return;
        SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
        const SDL_JoystickID id = SDL_JoystickInstanceID(joystick);
        const auto [position, inserted] = controllers_.emplace(id, controller);
        if (!inserted) {
            SDL_GameControllerClose(controller);
            static_cast<void>(position);
        }
    }

    std::unordered_map<SDL_JoystickID, SDL_GameController*> controllers_;
};

squared::application::Event::Key translate_key(SDL_Keycode key) noexcept
{
    using Key = squared::application::Event::Key;
    switch (key) {
    case SDLK_LEFT: return Key::left;
    case SDLK_RIGHT: return Key::right;
    case SDLK_UP: return Key::up;
    case SDLK_DOWN: return Key::down;
    case SDLK_HOME: return Key::home;
    case SDLK_END: return Key::end;
    case SDLK_BACKSPACE: return Key::backspace;
    case SDLK_DELETE: return Key::delete_key;
    case SDLK_RETURN:
    case SDLK_KP_ENTER: return Key::enter;
    case SDLK_SPACE: return Key::space;
    case SDLK_TAB: return Key::tab;
    case SDLK_ESCAPE: return Key::escape;
    default: return Key::unknown;
    }
}

squared::application::KeyModifiers translate_modifiers(Uint16 native) noexcept
{
    using Modifier = squared::application::KeyModifier;
    squared::application::KeyModifiers result;
    result.set(Modifier::shift, (native & KMOD_SHIFT) != 0);
    result.set(Modifier::control, (native & KMOD_CTRL) != 0);
    result.set(Modifier::alt, (native & KMOD_ALT) != 0);
    result.set(Modifier::meta, (native & KMOD_GUI) != 0);
    return result;
}

squared::application::Event::Navigation translate_navigation(
    Uint8 button
) noexcept
{
    using Navigation = squared::application::Event::Navigation;
    switch (button) {
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return Navigation::left;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return Navigation::right;
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return Navigation::up;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return Navigation::down;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return Navigation::next;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return Navigation::previous;
    case SDL_CONTROLLER_BUTTON_A: return Navigation::activate;
    case SDL_CONTROLLER_BUTTON_B: return Navigation::cancel;
    default: return Navigation::unknown;
    }
}

std::optional<squared::application::Event> translate_event(
    const SDL_Event& event,
    SdlTextInputService& text_input
) noexcept
{
    using Event = squared::application::Event;
    switch (event.type) {
    case SDL_QUIT:
        return Event{.type = Event::Type::QuitRequested};
    case SDL_FINGERDOWN:
    case SDL_FINGERMOTION:
    case SDL_FINGERUP:
        return Event{
            .type = event.type == SDL_FINGERDOWN ? Event::Type::PointerDown
                : event.type == SDL_FINGERMOTION ? Event::Type::PointerMove
                                                 : Event::Type::PointerUp,
            .pointer_id = static_cast<std::int64_t>(event.tfinger.fingerId),
            .x = event.tfinger.x * logical_width,
            .y = event.tfinger.y * logical_height
        };
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        if (event.button.which == SDL_TOUCH_MOUSEID) return std::nullopt;
        SDL_Window* window = SDL_GetWindowFromID(event.button.windowID);
        int width = 0;
        int height = 0;
        if (window) SDL_GetWindowSize(window, &width, &height);
        if (width <= 0 || height <= 0) return std::nullopt;
        return Event{
            .type = event.type == SDL_MOUSEBUTTONDOWN
                ? Event::Type::PointerDown : Event::Type::PointerUp,
            .pointer_id = static_cast<std::int64_t>(event.button.which),
            .x = static_cast<float>(event.button.x) * logical_width / width,
            .y = static_cast<float>(event.button.y) * logical_height / height
        };
    }
    case SDL_TEXTINPUT:
        return Event{.type = Event::Type::TextInput, .text = event.text.text};
    case SDL_TEXTEDITING:
        return Event{
            .type = Event::Type::TextEditing,
            .text = event.edit.text,
            .editing_start = event.edit.start,
            .editing_length = event.edit.length
        };
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_AC_BACK) {
            if (text_input.active()) {
                text_input.stop();
                return Event{.type = Event::Type::TextInputHidden};
            }
            return Event{.type = Event::Type::BackRequested};
        }
        return Event{
            .type = Event::Type::KeyDown,
            .key = translate_key(event.key.keysym.sym),
            .modifiers = translate_modifiers(event.key.keysym.mod),
            .repeat = event.key.repeat != 0
        };
    case SDL_KEYUP:
        return Event{
            .type = Event::Type::KeyUp,
            .key = translate_key(event.key.keysym.sym),
            .modifiers = translate_modifiers(event.key.keysym.mod)
        };
    case SDL_CONTROLLERBUTTONDOWN: {
        const auto navigation = translate_navigation(event.cbutton.button);
        if (navigation == Event::Navigation::unknown) return std::nullopt;
        return Event{
            .type = Event::Type::NavigationInput,
            .navigation = navigation,
            .input_device_id = event.cbutton.which
        };
    }
    case SDL_APP_WILLENTERBACKGROUND:
        return Event{.type = Event::Type::Pause};
    case SDL_APP_DIDENTERFOREGROUND:
        return Event{.type = Event::Type::Resume};
    case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
            event.window.event == SDL_WINDOWEVENT_RESIZED) {
            return Event{
                .type = Event::Type::Resize,
                .width = event.window.data1,
                .height = event.window.data2
            };
        }
        return std::nullopt;
    default:
        return std::nullopt;
    }
}

} // namespace

int main(int, char**)
{
    PlatformLibraries libraries;
    if (!libraries.initialize()) return 1;

    squared::graphics::Context graphics;
    if (!graphics.create("Squared GUI Showcase", logical_width, logical_height)) {
        SDL_Log("OpenGL ES graphics setup failed");
        return 1;
    }

    auto application = squared_gui_showcase::create_application();
    SdlTextInputService text_input;
    SdlControllers controllers;
    if (!application) {
        SDL_Log("Developer application factory failed");
        graphics.destroy();
        return 1;
    }
    application->set_text_input_service(&text_input);
    if (!application->create(graphics)) {
        SDL_Log("Developer application initialization failed");
        graphics.destroy();
        return 1;
    }
    application->surface_created(graphics);
    application->resize(graphics.pixel_width(), graphics.pixel_height());

    bool running = true;
    bool paused = false;
    bool surface_available = true;
    Uint64 previous_counter = SDL_GetPerformanceCounter();
    const Uint64 frequency = SDL_GetPerformanceFrequency();
    while (running && !application->quit_requested()) {
        SDL_Event native_event{};
        while (SDL_PollEvent(&native_event)) {
            controllers.handle(native_event);
            const auto event = translate_event(native_event, text_input);
            if (!event) continue;
            switch (event->type) {
            case squared::application::Event::Type::QuitRequested:
                running = false;
                break;
            case squared::application::Event::Type::Pause:
                if (!paused) {
                    paused = true;
                    application->pause();
                    if (text_input.active()) text_input.stop();
                    if (surface_available) {
                        application->surface_destroyed();
                        surface_available = false;
                    }
                }
                break;
            case squared::application::Event::Type::Resume:
                if (paused) {
                    if (!graphics.resume()) {
                        SDL_Log("OpenGL ES context restoration failed");
                        running = false;
                        break;
                    }
                    paused = false;
                    surface_available = true;
                    application->surface_created(graphics);
                    application->resize(
                        graphics.pixel_width(), graphics.pixel_height()
                    );
                    application->resume();
                    previous_counter = SDL_GetPerformanceCounter();
                }
                break;
            case squared::application::Event::Type::Resize:
                if (surface_available) {
                    graphics.refresh_viewport();
                    application->resize(
                        graphics.pixel_width(), graphics.pixel_height()
                    );
                }
                break;
            default: break;
            }
            application->handle_event(*event);
        }
        if (paused) { SDL_Delay(50); continue; }
        const Uint64 current = SDL_GetPerformanceCounter();
        const double seconds = static_cast<double>(current - previous_counter) /
            static_cast<double>(frequency);
        previous_counter = current;
        const auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::duration<double>(std::min(seconds, 0.1))
        );
        application->update(delta);
        application->render(graphics);
        graphics.present();
    }

    if (text_input.active()) text_input.stop();
    if (surface_available) application->surface_destroyed();
    application->dispose();
    application.reset();
    graphics.destroy();
    return 0;
}
