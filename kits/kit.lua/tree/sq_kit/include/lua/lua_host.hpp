// SPDX-License-Identifier: MIT
//
// lua/lua_host.hpp — Lua 5.4 bridge for the Squared framework.
//
// Contributed by kit.lua. Connects a Squared application to an embedded Lua
// 5.4 interpreter, so the generated project gets a `sq_lua/` script workspace
// alongside its `sq_app/` C++ workspace.
//
// GENERATED FILE. The generator owns this path and will replace it on update.
// Your scripts go in sq_lua/ and your C++ in sq_app/.
//
// Availability. This header compiles to nothing useful unless the build found
// Lua 5.4 -- mk/kit_lua.mk sets SQ_HAVE_LUA when it did. Without it, the class
// still exists and every method reports "lua unavailable" through the normal
// error channel. That is deliberate: a project that stops compiling because a
// system package is missing is much harder to diagnose than one that builds
// and tells you so at startup.

#ifndef SQUARED_KIT_LUA_HOST_HPP
#define SQUARED_KIT_LUA_HOST_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(SQ_HAVE_LUA)
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#endif

namespace sq::lua {

/// Whether this build has a working Lua interpreter behind it.
inline constexpr bool available =
#if defined(SQ_HAVE_LUA)
    true;
#else
    false;
#endif

/// The result of running a script or calling a function.
///
/// Errors are values rather than exceptions for the same reason the engine
/// makes that choice: a script failing is an ordinary event in a program that
/// runs user scripts, and the program should print it and carry on.
struct Outcome {
    bool        ok{false};
    std::string value;    ///< the returned value, rendered as text
    std::string error;    ///< empty when ok

    explicit operator bool() const noexcept { return ok; }
};

/// An embedded Lua interpreter.
///
/// Move-only and RAII: the state is closed by the destructor, never by the
/// caller. One Host owns one lua_State; sharing a state between two Hosts
/// would make ownership ambiguous for no gain.
class Host {
public:
    Host() {
#if defined(SQ_HAVE_LUA)
        state_ = luaL_newstate();
        if (state_ != nullptr) luaL_openlibs(state_);
#endif
    }

    ~Host() { close(); }

    Host(const Host&)            = delete;
    Host& operator=(const Host&) = delete;

    Host(Host&& other) noexcept {
#if defined(SQ_HAVE_LUA)
        state_ = std::exchange(other.state_, nullptr);
#else
        (void)other;
#endif
    }

    Host& operator=(Host&& other) noexcept {
        if (this != &other) {
            close();
#if defined(SQ_HAVE_LUA)
            state_ = std::exchange(other.state_, nullptr);
#endif
        }
        return *this;
    }

    /// Whether the interpreter is usable. False when Lua was not found at
    /// build time, or when the state could not be allocated.
    [[nodiscard]] bool ready() const noexcept {
#if defined(SQ_HAVE_LUA)
        return state_ != nullptr;
#else
        return false;
#endif
    }

    /// Add a directory to `package.path`, so `require` finds scripts there.
    /// Call this with your `sq_lua` directory before running anything.
    void add_script_path(const std::filesystem::path& directory) {
#if defined(SQ_HAVE_LUA)
        if (state_ == nullptr) return;
        const std::string addition =
            (directory / "?.lua").string() + ";" + (directory / "?" / "init.lua").string();

        lua_getglobal(state_, "package");
        lua_getfield(state_, -1, "path");
        const char* existing = lua_tostring(state_, -1);
        const std::string combined =
            existing != nullptr ? addition + ";" + existing : addition;
        lua_pop(state_, 1);
        lua_pushlstring(state_, combined.data(), combined.size());
        lua_setfield(state_, -2, "path");
        lua_pop(state_, 1);
#else
        (void)directory;
#endif
    }

    /// Run a script file.
    [[nodiscard]] Outcome run_file(const std::filesystem::path& path) {
#if defined(SQ_HAVE_LUA)
        if (state_ == nullptr) return unavailable();
        const int status = luaL_loadfile(state_, path.string().c_str());
        if (status != LUA_OK) return take_error();
        return call(0);
#else
        (void)path;
        return unavailable();
#endif
    }

    /// Run a chunk of source.
    [[nodiscard]] Outcome run(std::string_view source, std::string_view chunk_name = "=(sq)") {
#if defined(SQ_HAVE_LUA)
        if (state_ == nullptr) return unavailable();
        const int status =
            luaL_loadbuffer(state_, source.data(), source.size(), std::string{chunk_name}.c_str());
        if (status != LUA_OK) return take_error();
        return call(0);
#else
        (void)source;
        (void)chunk_name;
        return unavailable();
#endif
    }

    /// Call a global function with string arguments.
    ///
    /// String-only on purpose. A general marshaller belongs in the framework,
    /// not in a bridge header; strings cover the "call into a script and get
    /// text back" case a terminal application actually has, and anything more
    /// elaborate can use the state directly through native_state().
    [[nodiscard]] Outcome call_global(std::string_view name,
                                      const std::vector<std::string>& arguments = {}) {
#if defined(SQ_HAVE_LUA)
        if (state_ == nullptr) return unavailable();
        lua_getglobal(state_, std::string{name}.c_str());
        if (lua_isfunction(state_, -1) == 0) {
            lua_pop(state_, 1);
            return Outcome{false, {}, "no global function named '" + std::string{name} + "'"};
        }
        for (const std::string& argument : arguments) {
            lua_pushlstring(state_, argument.data(), argument.size());
        }
        return call(static_cast<int>(arguments.size()));
#else
        (void)name;
        (void)arguments;
        return unavailable();
#endif
    }

    /// Set a global string, for handing configuration to scripts.
    void set_global(std::string_view name, std::string_view value) {
#if defined(SQ_HAVE_LUA)
        if (state_ == nullptr) return;
        lua_pushlstring(state_, value.data(), value.size());
        lua_setglobal(state_, std::string{name}.c_str());
#else
        (void)name;
        (void)value;
#endif
    }

#if defined(SQ_HAVE_LUA)
    /// The underlying state, for code that needs the full Lua C API.
    ///
    /// Exposed rather than hidden: a bridge that boxes you in is worse than
    /// one that hands over the wheel when you ask. The state stays owned by
    /// this object -- do not close it.
    [[nodiscard]] lua_State* native_state() const noexcept { return state_; }
#endif

private:
    void close() noexcept {
#if defined(SQ_HAVE_LUA)
        if (state_ != nullptr) {
            lua_close(state_);
            state_ = nullptr;
        }
#endif
    }

    [[nodiscard]] static Outcome unavailable() {
        return Outcome{false, {},
                       "this build has no Lua interpreter; install Lua 5.4 development files and "
                       "rebuild (on Termux: pkg install lua54)"};
    }

#if defined(SQ_HAVE_LUA)
    [[nodiscard]] Outcome take_error() {
        const char* message = lua_tostring(state_, -1);
        Outcome     outcome{false, {}, message != nullptr ? message : "unknown Lua error"};
        lua_pop(state_, 1);
        return outcome;
    }

    [[nodiscard]] Outcome call(int argument_count) {
        if (lua_pcall(state_, argument_count, 1, 0) != LUA_OK) return take_error();
        Outcome outcome{true, {}, {}};
        if (lua_isnoneornil(state_, -1) == 0) {
            const char* text = lua_tostring(state_, -1);
            if (text != nullptr) outcome.value = text;
        }
        lua_pop(state_, 1);
        return outcome;
    }

    lua_State* state_{nullptr};
#endif
};

}  // namespace sq::lua

#endif  // SQUARED_KIT_LUA_HOST_HPP
