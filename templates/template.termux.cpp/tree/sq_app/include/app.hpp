// {{project_name}} — application interface.
//
// This file is YOURS. The generator seeded it once and will never overwrite
// it (ownership class: seeded). Rename it, split it, delete it — the build
// picks up every .cpp under sq_app/src/ automatically.

#ifndef {{project_name}}_APP_HPP
#define {{project_name}}_APP_HPP

#include <string>
#include <string_view>

namespace {{project_name}} {

/// The application. Owns whatever state your program needs.
///
/// `run` returns the process exit status, so `main` stays three lines and the
/// interesting code lives somewhere testable.
class App {
public:
    App();
    ~App();

    App(const App&)            = delete;
    App& operator=(const App&) = delete;

    /// Run to completion. Returns the exit status.
    [[nodiscard]] int run(int argc, char** argv);

private:
    [[nodiscard]] int interactive();
    [[nodiscard]] int handle(std::string_view line);

    struct State;
    State* state_;
};

}  // namespace {{project_name}}

#endif  // {{project_name}}_APP_HPP
