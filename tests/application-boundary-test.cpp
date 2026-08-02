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
};

}  // namespace

int main()
{
    static_assert(std::is_polymorphic_v<
        squared::application::Application
    >);

    squared::graphics::Context graphics;
    TestApplication application;
    assert(application.create(graphics));
    application.handle_event({
        .type =
            squared::application::Event::Type::PointerDown,
        .pointer_id = 7,
        .x = 12.0F,
        .y = 24.0F
    });
    application.update(std::chrono::milliseconds(16));
    application.render(graphics);
    application.dispose();

    assert(application.created);
    assert(application.rendered);
    assert(application.disposed);
    assert(application.delta == std::chrono::milliseconds(16));
    assert(
        application.last_event ==
        squared::application::Event::Type::PointerDown
    );
}
