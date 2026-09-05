// SPDX-License-Identifier: MIT
//
// squared/kit/termux.hpp — Termux:API bridge for the Squared framework.
//
// Contributed by kit.termux. A *bridge*: it connects a Squared application to
// the Android facilities Termux exposes — battery, clipboard, notifications,
// dialogs, speech, location, sensors — in the same way kit.opengl connects one
// to EGL.
//
// GENERATED FILE. The generator owns this path and will replace it on update.
// Write your own code in the working directory instead.
//
// ---------------------------------------------------------------------------
// How this works, and what it costs
// ---------------------------------------------------------------------------
//
// Termux:API is not a library. It is a set of command-line programs that talk
// to a companion Android app over a socket and print JSON. There is no C API
// to link against, so this kit runs those programs and parses their output.
//
// That has consequences worth stating plainly rather than hiding:
//
//   * every call forks a process. Milliseconds, not microseconds. Fine for a
//     notification; wrong inside a frame loop.
//   * every call can fail because the *app* is missing, which is separate from
//     the `termux-api` package being installed. Both are checked.
//   * arguments are shell-quoted here. Do not build command strings yourself.
//
// The alternative — declaring this too ugly to support — would leave every
// user writing the same popen-and-parse code, less carefully.
//
// ---------------------------------------------------------------------------
// Requirements
// ---------------------------------------------------------------------------
//
//   pkg install termux-api          the command-line programs
//   plus the Termux:API app         from F-Droid; the package alone does nothing
//
// `available()` reports the first, `probe()` reports both.

#ifndef SQUARED_KIT_TERMUX_HPP
#define SQUARED_KIT_TERMUX_HPP

#include <array>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sq::termux {

// ---------------------------------------------------------------------------
// JSON
//
// A small, complete parser. Vendoring one would be a dependency; asking the
// caller to parse would mean everyone writes the same thing badly. termux-api
// output is the only reason this kit exists, and it is all JSON.
// ---------------------------------------------------------------------------

class Json {
public:
    enum class Kind { null, boolean, number, string, array, object };

    Json() = default;

    /// Parse. Empty optional on malformed input; no exceptions, no partial
    /// values.
    [[nodiscard]] static std::optional<Json> parse(std::string_view text) {
        std::size_t index = 0;
        auto        value = parse_value(text, index);
        if (!value) return std::nullopt;
        skip_space(text, index);
        if (index != text.size()) return std::nullopt;  // trailing garbage
        return value;
    }

    [[nodiscard]] Kind kind() const noexcept { return kind_; }
    [[nodiscard]] bool is_null() const noexcept { return kind_ == Kind::null; }

    [[nodiscard]] std::optional<bool> as_bool() const {
        if (kind_ != Kind::boolean) return std::nullopt;
        return boolean_;
    }
    [[nodiscard]] std::optional<double> as_number() const {
        if (kind_ != Kind::number) return std::nullopt;
        return number_;
    }
    [[nodiscard]] std::optional<long long> as_int() const {
        if (kind_ != Kind::number) return std::nullopt;
        return static_cast<long long>(number_);
    }
    [[nodiscard]] std::optional<std::string> as_string() const {
        if (kind_ != Kind::string) return std::nullopt;
        return string_;
    }

    /// Object member, or null when absent. Chains safely:
    /// `root["a"]["b"].as_string()` on missing `a` yields an empty optional
    /// rather than a crash.
    [[nodiscard]] const Json& operator[](std::string_view key) const {
        static const Json kAbsent;
        if (kind_ != Kind::object) return kAbsent;
        for (const auto& [name, value] : members_) {
            if (name == key) return value;
        }
        return kAbsent;
    }

    [[nodiscard]] const Json& operator[](std::size_t index) const {
        static const Json kAbsent;
        if (kind_ != Kind::array || index >= elements_.size()) return kAbsent;
        return elements_[index];
    }

    [[nodiscard]] std::size_t size() const noexcept {
        if (kind_ == Kind::array) return elements_.size();
        if (kind_ == Kind::object) return members_.size();
        return 0;
    }

    [[nodiscard]] std::vector<std::string> keys() const {
        std::vector<std::string> out;
        out.reserve(members_.size());
        for (const auto& [name, value] : members_) out.push_back(name);
        return out;
    }

    // Convenience readers with fallbacks, for the common "read one field"
    // case that would otherwise be three lines every time.
    [[nodiscard]] std::string string_or(std::string_view key, std::string fallback = {}) const {
        auto value = (*this)[key].as_string();
        return value ? *value : std::move(fallback);
    }
    [[nodiscard]] double number_or(std::string_view key, double fallback = 0.0) const {
        auto value = (*this)[key].as_number();
        return value ? *value : fallback;
    }
    [[nodiscard]] long long int_or(std::string_view key, long long fallback = 0) const {
        auto value = (*this)[key].as_int();
        return value ? *value : fallback;
    }
    [[nodiscard]] bool bool_or(std::string_view key, bool fallback = false) const {
        auto value = (*this)[key].as_bool();
        return value ? *value : fallback;
    }

private:
    static void skip_space(std::string_view text, std::size_t& i) {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t' || text[i] == '\n' ||
                                   text[i] == '\r')) {
            ++i;
        }
    }

    static bool literal(std::string_view text, std::size_t& i, std::string_view word) {
        if (text.compare(i, word.size(), word) != 0) return false;
        i += word.size();
        return true;
    }

    static std::optional<std::string> parse_string(std::string_view text, std::size_t& i) {
        if (i >= text.size() || text[i] != '"') return std::nullopt;
        ++i;
        std::string out;
        while (i < text.size()) {
            const char c = text[i];
            if (c == '"') {
                ++i;
                return out;
            }
            if (c != '\\') {
                out.push_back(c);
                ++i;
                continue;
            }
            if (++i >= text.size()) return std::nullopt;
            switch (text[i]) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    if (i + 4 >= text.size()) return std::nullopt;
                    unsigned code = 0;
                    for (int k = 1; k <= 4; ++k) {
                        const char h = text[i + static_cast<std::size_t>(k)];
                        code <<= 4;
                        if (h >= '0' && h <= '9') code |= static_cast<unsigned>(h - '0');
                        else if (h >= 'a' && h <= 'f') code |= static_cast<unsigned>(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') code |= static_cast<unsigned>(h - 'A' + 10);
                        else return std::nullopt;
                    }
                    i += 4;
                    // UTF-8 encode. Surrogate pairs are not recombined:
                    // termux-api emits plain UTF-8 and escapes little, so the
                    // extra state is not worth the code. Lone surrogates become
                    // the replacement character rather than invalid UTF-8.
                    if (code >= 0xD800 && code <= 0xDFFF) code = 0xFFFD;
                    if (code < 0x80) {
                        out.push_back(static_cast<char>(code));
                    } else if (code < 0x800) {
                        out.push_back(static_cast<char>(0xC0 | (code >> 6)));
                        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                    } else {
                        out.push_back(static_cast<char>(0xE0 | (code >> 12)));
                        out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                    }
                    break;
                }
                default: return std::nullopt;
            }
            ++i;
        }
        return std::nullopt;  // unterminated
    }

    static std::optional<Json> parse_value(std::string_view text, std::size_t& i) {
        skip_space(text, i);
        if (i >= text.size()) return std::nullopt;

        Json value;
        switch (text[i]) {
            case 'n':
                if (!literal(text, i, "null")) return std::nullopt;
                value.kind_ = Kind::null;
                return value;
            case 't':
                if (!literal(text, i, "true")) return std::nullopt;
                value.kind_ = Kind::boolean;
                value.boolean_ = true;
                return value;
            case 'f':
                if (!literal(text, i, "false")) return std::nullopt;
                value.kind_ = Kind::boolean;
                value.boolean_ = false;
                return value;
            case '"': {
                auto text_value = parse_string(text, i);
                if (!text_value) return std::nullopt;
                value.kind_ = Kind::string;
                value.string_ = std::move(*text_value);
                return value;
            }
            case '[': {
                ++i;
                value.kind_ = Kind::array;
                skip_space(text, i);
                if (i < text.size() && text[i] == ']') {
                    ++i;
                    return value;
                }
                while (true) {
                    auto element = parse_value(text, i);
                    if (!element) return std::nullopt;
                    value.elements_.push_back(std::move(*element));
                    skip_space(text, i);
                    if (i >= text.size()) return std::nullopt;
                    if (text[i] == ',') {
                        ++i;
                        continue;
                    }
                    if (text[i] == ']') {
                        ++i;
                        return value;
                    }
                    return std::nullopt;
                }
            }
            case '{': {
                ++i;
                value.kind_ = Kind::object;
                skip_space(text, i);
                if (i < text.size() && text[i] == '}') {
                    ++i;
                    return value;
                }
                while (true) {
                    skip_space(text, i);
                    auto key = parse_string(text, i);
                    if (!key) return std::nullopt;
                    skip_space(text, i);
                    if (i >= text.size() || text[i] != ':') return std::nullopt;
                    ++i;
                    auto member = parse_value(text, i);
                    if (!member) return std::nullopt;
                    value.members_.emplace_back(std::move(*key), std::move(*member));
                    skip_space(text, i);
                    if (i >= text.size()) return std::nullopt;
                    if (text[i] == ',') {
                        ++i;
                        continue;
                    }
                    if (text[i] == '}') {
                        ++i;
                        return value;
                    }
                    return std::nullopt;
                }
            }
            default: {
                const std::size_t start = i;
                if (i < text.size() && (text[i] == '-' || text[i] == '+')) ++i;
                while (i < text.size() && (std::isdigit(static_cast<unsigned char>(text[i])) != 0 ||
                                           text[i] == '.' || text[i] == 'e' || text[i] == 'E' ||
                                           text[i] == '-' || text[i] == '+')) {
                    ++i;
                }
                if (i == start) return std::nullopt;
                // strtod rather than from_chars: libc++ on Android did not
                // implement the floating-point overload until recently, and a
                // kit that fails to compile on the platform it targets is not
                // much of a kit.
                const std::string token{text.substr(start, i - start)};
                char*             end = nullptr;
                const double      parsed = std::strtod(token.c_str(), &end);
                if (end == token.c_str()) return std::nullopt;
                value.kind_ = Kind::number;
                value.number_ = parsed;
                return value;
            }
        }
    }

    Kind        kind_{Kind::null};
    bool        boolean_{false};
    double      number_{0.0};
    std::string string_;
    std::vector<Json>                             elements_;
    std::vector<std::pair<std::string, Json>>     members_;
};

// ---------------------------------------------------------------------------
// Running a termux-api command
// ---------------------------------------------------------------------------

/// The outcome of a command.
struct Result {
    bool        ok{false};
    std::string output;   ///< stdout, verbatim
    std::string error;    ///< why not, when !ok

    explicit operator bool() const noexcept { return ok; }

    /// stdout parsed as JSON. Empty when the command failed or printed
    /// something else.
    [[nodiscard]] std::optional<Json> json() const {
        if (!ok) return std::nullopt;
        return Json::parse(output);
    }
};

namespace detail {

/// Wrap one argument for the shell.
///
/// Single quotes, with an embedded quote closed and reopened. This is the only
/// quoting rule in POSIX sh with no escapes inside it, which is exactly why it
/// is the one to use: there is nothing to get subtly wrong.
///
/// Every argument goes through here. A caller who builds a command string by
/// concatenation has an injection bug, and text from a clipboard or a dialog is
/// precisely the text most likely to contain a quote.
[[nodiscard]] inline std::string shell_quote(std::string_view argument) {
    std::string out;
    out.reserve(argument.size() + 2);
    out.push_back('\'');
    for (char c : argument) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('\'');
    return out;
}

[[nodiscard]] inline bool command_exists(std::string_view name) {
    const std::string probe = "command -v " + shell_quote(name) + " >/dev/null 2>&1";
    return std::system(probe.c_str()) == 0;
}

}  // namespace detail

/// Whether the termux-api command-line programs are installed.
///
/// Not the same as them working: the programs are a thin client for the
/// Termux:API *app*, and without the app they hang or return nothing. See
/// probe().
[[nodiscard]] inline bool available() { return detail::command_exists("termux-battery-status"); }

/// Run a termux-api command with arguments.
///
/// Arguments are quoted; pass them as separate strings and never pre-quote.
/// stderr is folded into stdout, because termux-api reports failures there and
/// discarding it would turn a diagnosable error into an empty result.
[[nodiscard]] inline Result run(std::string_view command,
                                const std::vector<std::string>& arguments = {}) {
    if (!detail::command_exists(command)) {
        return Result{false, {},
                      std::string{command} +
                          " not found. Install the programs and the app:\n"
                          "  pkg install termux-api\n"
                          "  plus the Termux:API app from F-Droid"};
    }

    std::string line{command};
    for (const std::string& argument : arguments) {
        line += ' ';
        line += detail::shell_quote(argument);
    }
    line += " 2>&1";

    std::FILE* pipe = ::popen(line.c_str(), "r");
    if (pipe == nullptr) return Result{false, {}, "could not start " + std::string{command}};

    std::string           output;
    std::array<char, 4096> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    const int status = ::pclose(pipe);

    if (status != 0) {
        return Result{false, output,
                      std::string{command} + " exited with status " + std::to_string(status)};
    }
    return Result{true, std::move(output), {}};
}

/// A fuller check than available(): confirms the programs exist *and* that the
/// app answers.
///
/// Costs a real round trip, so call it once at startup rather than before each
/// use. Battery status is the probe because it needs no permission and no user
/// interaction.
[[nodiscard]] inline Result probe() {
    if (!available()) {
        return Result{false, {},
                      "termux-api programs are not installed (pkg install termux-api)"};
    }
    Result result = run("termux-battery-status");
    if (result.ok && !result.json()) {
        return Result{false, result.output,
                      "termux-api is installed but returned no JSON. The Termux:API app is "
                      "probably missing — install it from F-Droid."};
    }
    return result;
}

// ---------------------------------------------------------------------------
// Typed conveniences
//
// A thin layer over run(). Anything not covered here is one run() call away,
// and the raw Result is always available.
// ---------------------------------------------------------------------------

struct Battery {
    int         percentage{0};
    std::string status;       ///< CHARGING, DISCHARGING, FULL, NOT_CHARGING, UNKNOWN
    std::string health;
    double      temperature{0.0};
    bool        plugged{false};
};

[[nodiscard]] inline std::optional<Battery> battery() {
    auto json = run("termux-battery-status").json();
    if (!json) return std::nullopt;

    Battery out;
    out.percentage  = static_cast<int>(json->int_or("percentage"));
    out.status      = json->string_or("status");
    out.health      = json->string_or("health");
    out.temperature = json->number_or("temperature");
    out.plugged     = out.status == "CHARGING" || out.status == "FULL";
    return out;
}

[[nodiscard]] inline std::optional<std::string> clipboard_get() {
    Result result = run("termux-clipboard-get");
    if (!result) return std::nullopt;
    // The clipboard is raw text, not JSON, and a trailing newline is added by
    // the tool rather than being part of the content.
    std::string text = std::move(result.output);
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
    return text;
}

[[nodiscard]] inline bool clipboard_set(std::string_view text) {
    return run("termux-clipboard-set", {std::string{text}}).ok;
}

/// A brief on-screen message. `position` is top, middle or bottom.
[[nodiscard]] inline bool toast(std::string_view text, std::string_view position = "bottom") {
    return run("termux-toast", {"-g", std::string{position}, std::string{text}}).ok;
}

/// A notification in the shade. `id` lets a later call replace this one rather
/// than stacking a second copy.
[[nodiscard]] inline bool notify(std::string_view title, std::string_view content,
                                 std::string_view id = {}) {
    std::vector<std::string> arguments{"--title", std::string{title},
                                       "--content", std::string{content}};
    if (!id.empty()) {
        arguments.emplace_back("--id");
        arguments.emplace_back(id);
    }
    return run("termux-notification", arguments).ok;
}

[[nodiscard]] inline bool notification_remove(std::string_view id) {
    return run("termux-notification-remove", {std::string{id}}).ok;
}

/// Vibrate. `force` overrides the device being in silent mode.
[[nodiscard]] inline bool vibrate(int milliseconds = 200, bool force = false) {
    std::vector<std::string> arguments{"-d", std::to_string(milliseconds)};
    if (force) arguments.emplace_back("-f");
    return run("termux-vibrate", arguments).ok;
}

/// Ask for a line of text in a native dialog. Empty optional if cancelled.
[[nodiscard]] inline std::optional<std::string> dialog_text(std::string_view title,
                                                            std::string_view hint = {}) {
    std::vector<std::string> arguments{"-t", std::string{title}};
    if (!hint.empty()) {
        arguments.emplace_back("-i");
        arguments.emplace_back(hint);
    }
    auto json = run("termux-dialog", arguments).json();
    if (!json) return std::nullopt;
    if (json->int_or("code", -1) != -1) return std::nullopt;  // -1 is OK; 0 and 1 are cancel
    return json->string_or("text");
}

/// Ask the user to pick one of `options`. Empty optional if cancelled.
[[nodiscard]] inline std::optional<std::string> dialog_choice(
    std::string_view title, const std::vector<std::string>& options) {
    std::string joined;
    for (std::size_t i = 0; i < options.size(); ++i) {
        if (i > 0) joined += ',';
        joined += options[i];
    }
    auto json = run("termux-dialog", {"radio", "-t", std::string{title}, "-v", joined}).json();
    if (!json) return std::nullopt;
    if (json->int_or("code", -1) != -1) return std::nullopt;
    return json->string_or("text");
}

[[nodiscard]] inline bool speak(std::string_view text) {
    return run("termux-tts-speak", {std::string{text}}).ok;
}

struct Location {
    double latitude{0.0};
    double longitude{0.0};
    double accuracy{0.0};
    double altitude{0.0};
};

/// A location fix. `provider` is gps, network or passive.
///
/// Slow and permission-gated; gps in particular can block for many seconds
/// outdoors and forever indoors. Never call this on a thread that has to stay
/// responsive.
[[nodiscard]] inline std::optional<Location> location(std::string_view provider = "network") {
    auto json = run("termux-location", {"-p", std::string{provider}}).json();
    if (!json) return std::nullopt;
    if (!(*json)["latitude"].as_number()) return std::nullopt;  // an error object

    Location out;
    out.latitude  = json->number_or("latitude");
    out.longitude = json->number_or("longitude");
    out.accuracy  = json->number_or("accuracy");
    out.altitude  = json->number_or("altitude");
    return out;
}

/// Hand a file to Android's share sheet.
[[nodiscard]] inline bool share(std::string_view path, std::string_view title = {}) {
    std::vector<std::string> arguments{"-a", "send"};
    if (!title.empty()) {
        arguments.emplace_back("-t");
        arguments.emplace_back(title);
    }
    arguments.emplace_back(std::string{path});
    return run("termux-share", arguments).ok;
}

/// Open a file or URL with the system handler. Part of core Termux rather than
/// Termux:API, so it works without the companion app.
[[nodiscard]] inline bool open(std::string_view path_or_url) {
    return run("termux-open", {std::string{path_or_url}}).ok;
}

[[nodiscard]] inline bool torch(bool on) {
    return run("termux-torch", {on ? "on" : "off"}).ok;
}

/// Raw JSON from any sensor, e.g. accelerometer or light.
/// `count` samples then stops; without it the command runs forever.
[[nodiscard]] inline std::optional<Json> sensor(std::string_view name, int count = 1) {
    return run("termux-sensor", {"-s", std::string{name}, "-n", std::to_string(count)}).json();
}

[[nodiscard]] inline std::optional<Json> wifi_info() {
    return run("termux-wifi-connectioninfo").json();
}

[[nodiscard]] inline std::optional<Json> telephony_info() {
    return run("termux-telephony-deviceinfo").json();
}

}  // namespace sq::termux

#endif  // SQUARED_KIT_TERMUX_HPP
