// {{project_name}} — application implementation.
//
// This file is YOURS (ownership class: seeded). Start here.
//
// What the generator gave you:
//
//   sq::term    from kit.terminal — console I/O, regex helpers, FileHandle.
//               Header-only, no dependencies beyond the standard library.
//   sq::lua     from kit.lua, if you selected it — an embedded Lua 5.4
//               interpreter running the scripts in sq_lua/.
//
// The kit.lua include is guarded, so this file compiles whether or not the kit
// was applied. That is what lets you add the kit later without editing code
// you have already written.

#include "app.hpp"

#include <squared/kit/terminal.hpp>

#if __has_include(<squared/kit/lua_host.hpp>)
#  include <squared/kit/lua_host.hpp>
#  define {{project_name}}_HAS_LUA 1
#endif

#include <cctype>
#include <string>
#include <vector>

namespace {{project_name}} {
namespace {

constexpr std::string_view kName = "{{project_name}}";

}  // namespace

/// Private state, so app.hpp stays free of implementation detail and adding a
/// member does not force everything that includes it to rebuild.
struct App::State {
    std::vector<std::string> arguments;
    bool                     scripted{false};
#if defined({{project_name}}_HAS_LUA)
    sq::lua::Host lua;
#endif
};

App::App() : state_(new State{}) {}

App::~App() { delete state_; }

int App::run(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) state_->arguments.emplace_back(argv[i]);

#if defined({{project_name}}_HAS_LUA)
    // Scripts live beside the binary's project root. Adding the path before
    // running main.lua is what lets scripts `require` each other.
    state_->lua.add_script_path("sq_lua");
    if (state_->lua.ready()) {
        const auto loaded = state_->lua.run_file("sq_lua/main.lua");
        if (loaded) {
            state_->scripted = true;
            const auto greeting = state_->lua.call_global("sq_greet", {std::string{kName}});
            if (greeting) {
                sq::term::println(greeting.value);
            } else {
                sq::term::eprintln("script: ", greeting.error);
            }
        } else {
            sq::term::eprintln("script: ", loaded.error);
        }
    } else {
        sq::term::eprintln("script: this build has no Lua interpreter");
        sq::term::eprintln("        run `make lua-status` for details");
    }
#endif

    // A single argument is treated as a one-shot command, so the program is
    // useful in a pipe as well as at a prompt.
    if (!state_->arguments.empty()) {
        int status = 0;
        for (const std::string& argument : state_->arguments) {
            status = handle(argument);
            if (status != 0) return status;
        }
        return status;
    }

    return interactive();
}

int App::interactive() {
    sq::term::println(kName, " — type `help` for commands, `quit` to leave");

    while (true) {
        const auto line = sq::term::input("> ");
        // No line means end of input, not an empty line. Treating the two the
        // same would spin forever under `{{project_name}} < /dev/null`.
        if (!line) {
            sq::term::println();
            return 0;
        }

        const std::string trimmed = sq::term::trim(*line);
        if (trimmed.empty()) continue;
        if (trimmed == "quit" || trimmed == "exit") return 0;

        if (handle(trimmed) != 0) return 1;
    }
}

int App::handle(std::string_view line) {
    std::string text{line};

#if defined({{project_name}}_HAS_LUA)
    // Give the script workspace first refusal on every line. This is the hook
    // that makes sq_lua/ worth having: behaviour changes without a rebuild.
    if (state_->scripted) {
        const auto transformed = state_->lua.call_global("sq_transform", {text});
        if (transformed && !transformed.value.empty()) text = transformed.value;
    }
#endif

    if (text == "help") {
        sq::term::println("commands:");
        sq::term::println("  help              this text");
        sq::term::println("  echo <words>      print the rest of the line");
        sq::term::println("  upper <words>     print it in upper case");
        sq::term::println("  words <text>      split on whitespace and number the fields");
        sq::term::println("  read <path>       print a file");
        sq::term::println("  write <path>      read a line and write it to the file");
        sq::term::println("  quit              leave");
        return 0;
    }

    // sq::term's regex facade: capture the verb and the rest of the line in
    // one call, with no std::smatch boilerplate at the call site.
    const auto parts = sq::term::find(text, R"(^(\w+)\s*(.*)$)");
    const std::string verb = parts.size() > 1 ? parts[1] : text;
    const std::string rest = parts.size() > 2 ? parts[2] : std::string{};

    if (verb == "echo") {
        sq::term::println(rest);
        return 0;
    }

    if (verb == "upper") {
        // Replace every lower-case run with itself, then upper it by hand:
        // std::regex has no case-folding replacement, and doing it explicitly
        // is clearer than a clever pattern.
        std::string upper = rest;
        for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        sq::term::println(upper);
        return 0;
    }

    if (verb == "words") {
        const auto fields = sq::term::split(rest, R"(\s+)");
        int index = 1;
        for (const std::string& field : fields) {
            if (!field.empty()) sq::term::println("  ", index++, ": ", field);
        }
        return 0;
    }

    if (verb == "read") {
        const sq::term::FileHandle handle{rest};
        const auto content = handle.readString();
        if (!content) {
            sq::term::eprintln("cannot read ", handle);
            return 0;
        }
        sq::term::print(*content);
        if (!content->empty() && content->back() != '\n') sq::term::println();
        return 0;
    }

    if (verb == "write") {
        const auto content = sq::term::input("content> ");
        if (!content) return 0;
        if (!sq::term::FileHandle::writeString(rest, *content + "\n")) {
            sq::term::eprintln("cannot write ", rest);
            return 0;
        }
        sq::term::println("wrote ", content->size() + 1, " bytes to ", rest);
        return 0;
    }

    sq::term::eprintln("unknown command: ", verb, " (try `help`)");
    return 0;
}

}  // namespace {{project_name}}
