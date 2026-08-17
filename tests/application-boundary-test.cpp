#include <squared/application/application.hpp>

#include <cassert>
#include <chrono>
#include <type_traits>

namespace squared::graphics {
class Context {};
}

namespace {

class TestApplication final : public squared::application::Application {
public:
    void set_text_input_service(
        squared::application::TextInputService* value
    ) noexcept override
    {
        text_input_service = value;
    }
    [[nodiscard]] bool create(
        squared::graphics::Context&
    ) override
    {
        created = true;
        return true;
    }

    void handle_event(
        const squared::application::Event& event
    ) override
    {
        last_event = event.type;
    }

    void update(std::chrono::nanoseconds value) override
    {
        delta = value;
    }

    void render(squared::graphics::Context&) override
    {
        rendered = true;
    }

    void surface_created(squared::graphics::Context&) override
    {
        ++surface_creations;
    }

    void surface_destroyed() override
    {
        ++surface_destructions;
    }

    void dispose() override
    {
        disposed = true;
    }

    [[nodiscard]] bool quit_requested() const noexcept override
    {
        return false;
    }

    bool created{false};
    bool rendered{false};
    bool disposed{false};
    std::chrono::nanoseconds delta{0};
    squared::application::Event::Type last_event{
        squared::application::Event::Type::QuitRequested
    };
    squared::application::TextInputService* text_input_service{nullptr};
    int surface_creations{0};
    int surface_destructions{0};
};

class TestTextInput final : public squared::application::TextInputService {
public:
    void start(const squared::application::TextInputRequest&) override
    { active_state = true; }
    void update_area(const squared::application::TextInputArea&) override {}
    void stop() override { active_state = false; }
    [[nodiscard]] bool active() const noexcept override { return active_state; }
    bool active_state{false};
};

}  // namespace

int main()
{
    static_assert(std::is_polymorphic_v<
        squared::application::Application
    >);
    constexpr auto combined_modifiers =
        squared::application::KeyModifier::shift |
        squared::application::KeyModifier::control;
    static_assert(combined_modifiers.contains(
        squared::application::KeyModifier::shift
    ));
    static_assert(combined_modifiers.contains(
        squared::application::KeyModifier::control
    ));

    squared::graphics::Context graphics;
    TestApplication application;
    TestTextInput text_input;
    application.set_text_input_service(&text_input);
    assert(application.text_input_service == &text_input);
    assert(application.create(graphics));
    application.surface_created(graphics);
    application.surface_destroyed();
    application.surface_created(graphics);
    application.handle_event({
        .type =
            squared::application::Event::Type::PointerDown,
        .pointer_id = 7,
        .x = 12.0F,
        .y = 24.0F,
        .modifiers = squared::application::KeyModifiers{},
        .text = {}
    });
    application.update(std::chrono::milliseconds(16));
    application.render(graphics);
    application.surface_destroyed();
    application.dispose();

    assert(application.created);
    assert(application.rendered);
    assert(application.disposed);
    assert(application.delta == std::chrono::milliseconds(16));
    assert(application.surface_creations == 2);
    assert(application.surface_destructions == 2);
    assert(
        application.last_event ==
        squared::application::Event::Type::PointerDown
    );

    squared::application::Event navigation{
        .type = squared::application::Event::Type::NavigationInput,
        .modifiers = squared::application::KeyModifiers{},
        .navigation = squared::application::Event::Navigation::activate,
        .input_device_id = 4,
        .text = {}
    };
    assert(
        navigation.navigation ==
            squared::application::Event::Navigation::activate &&
        navigation.input_device_id == 4
    );
}
