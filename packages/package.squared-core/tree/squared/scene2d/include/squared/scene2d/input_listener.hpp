#pragma once

#include <functional>

namespace sq::scene2d {

class InputEvent;

/** @brief Callback invoked for each phase during input dispatch. */
using InputListener = std::function<void(InputEvent&)>;

} // namespace sq::scene2d
