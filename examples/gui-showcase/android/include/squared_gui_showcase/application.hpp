#pragma once

#include <memory>

namespace squared::application {
class Application;
}

namespace squared_gui_showcase {

std::unique_ptr<squared::application::Application> create_application();

} // namespace squared_gui_showcase
