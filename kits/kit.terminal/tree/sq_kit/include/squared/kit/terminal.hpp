// SPDX-License-Identifier: MIT
//
// squared/kit/terminal.hpp — terminal bridge for the Squared framework.
//
// Contributed by kit.terminal. This is a *bridge*: it connects a Squared
// application to the host terminal, in the same way kit.sdl3 would connect one
// to SDL3 (Generator Architecture §2.9.1). It is header-only and depends on
// nothing but the C++20 standard library, so a generated workspace builds with
// a compiler and make and nothing else.
//
// GENERATED FILE. The generator owns this path and will replace it on update.
// Write your own code in the working directory instead.
//
// Everything lives in `sq::term`. The three groups are:
//
//   output/input   print, println, input, prompt, confirm
//   text           a small regex facade over <regex>
//   files          FileHandle, modelled on libGDX's class of the same name
//
// Design note. This deliberately does not manage the terminal: no raw mode, no
// cursor control, no alternate screen. That is a TUI's job and belongs to
// kit.ncurses. A kit that quietly grew into a TUI would leave a user who chose
// "simple terminal application" with a dependency they did not ask for.

#ifndef SQUARED_KIT_TERMINAL_HPP
#define SQUARED_KIT_TERMINAL_HPP

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace sq::term {

// ---------------------------------------------------------------------------
// Output
// ---------------------------------------------------------------------------

/// Write every argument to stdout, with no separator and no newline.
///
/// Variadic rather than single-argument because the alternative is a stream of
/// `<<` at every call site, and the point of a kit is to make the common thing
/// short.
template <class... Args>
void print(const Args&... args) {
    (std::cout << ... << args);
}

/// Like print, followed by a newline.
template <class... Args>
void println(const Args&... args) {
    (std::cout << ... << args) << '\n';
}

/// Write to stderr. Diagnostics belong there, so that a program's real output
/// stays pipeable.
template <class... Args>
void eprintln(const Args&... args) {
    (std::cerr << ... << args) << '\n';
}

/// Flush stdout. Needed before reading input when a prompt has no newline.
inline void flush() { std::cout.flush(); }

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

/// Read one line from stdin, without the trailing newline.
///
/// Returns an empty optional at end of input, which is *not* the same as an
/// empty line. A caller that ignores the distinction gets an infinite loop the
/// first time its program is run with input redirected from /dev/null.
[[nodiscard]] inline std::optional<std::string> input() {
    std::string line;
    if (!std::getline(std::cin, line)) return std::nullopt;
    if (!line.empty() && line.back() == '\r') line.pop_back();  // tolerate CRLF
    return line;
}

/// Write a prompt, then read a line.
[[nodiscard]] inline std::optional<std::string> input(std::string_view prompt) {
    std::cout << prompt;
    flush();
    return input();
}

/// Read a line, returning `fallback` at end of input or on an empty line.
[[nodiscard]] inline std::string prompt(std::string_view message, std::string fallback = {}) {
    if (!fallback.empty()) {
        std::cout << message << " [" << fallback << "] ";
    } else {
        std::cout << message << ' ';
    }
    flush();
    auto line = input();
    if (!line || line->empty()) return fallback;
    return *line;
}

/// Ask a yes/no question. End of input yields `fallback`, so a piped program
/// does not hang waiting for an answer nobody is there to give.
[[nodiscard]] inline bool confirm(std::string_view message, bool fallback = false) {
    std::cout << message << (fallback ? " [Y/n] " : " [y/N] ");
    flush();
    auto line = input();
    if (!line || line->empty()) return fallback;
    const char first = static_cast<char>(std::tolower(static_cast<unsigned char>(line->front())));
    return first == 'y';
}

/// Read the whole of stdin. For programs used in a pipe.
[[nodiscard]] inline std::string read_all_input() {
    std::ostringstream buffer;
    buffer << std::cin.rdbuf();
    return buffer.str();
}

/// Parse a string as a number, returning nothing rather than throwing.
///
/// `std::stoi` throws on bad input and silently truncates trailing garbage;
/// from_chars does neither, which is what makes it right for parsing anything
/// a user typed.
template <class T>
[[nodiscard]] std::optional<T> parse(std::string_view text) {
    T value{};
    const char* first = text.data();
    const char* last  = text.data() + text.size();
    const auto  result = std::from_chars(first, last, value);
    if (result.ec != std::errc{} || result.ptr != last) return std::nullopt;
    return value;
}

// ---------------------------------------------------------------------------
// Text
//
// A facade over <regex>, not a replacement for it. std::regex is verbose at
// the call site and throws on a bad pattern; these four functions cover what a
// terminal program actually needs and never throw. `pattern()` is there for
// the cases that outgrow them.
// ---------------------------------------------------------------------------

/// ECMAScript syntax, which is std::regex's default and the one most people
/// already know from JavaScript and Python.
[[nodiscard]] inline std::optional<std::regex> pattern(std::string_view expression,
                                                       bool ignore_case = false) {
    auto flags = std::regex::ECMAScript;
    if (ignore_case) flags |= std::regex::icase;
    try {
        return std::regex{std::string{expression}, flags};
    } catch (const std::regex_error&) {
        // A bad pattern is a programming error, but it is frequently a
        // programming error in a pattern the *user* supplied, and a terminal
        // program should report that rather than terminate.
        return std::nullopt;
    }
}

/// Whether the whole string matches.
[[nodiscard]] inline bool matches(std::string_view text, std::string_view expression,
                                  bool ignore_case = false) {
    auto compiled = pattern(expression, ignore_case);
    if (!compiled) return false;
    return std::regex_match(std::string{text}, *compiled);
}

/// Whether the pattern occurs anywhere in the string.
[[nodiscard]] inline bool contains(std::string_view text, std::string_view expression,
                                   bool ignore_case = false) {
    auto compiled = pattern(expression, ignore_case);
    if (!compiled) return false;
    return std::regex_search(std::string{text}, *compiled);
}

/// The first match, with its capture groups.
///
/// Index 0 is the whole match and 1..n are the groups, matching every other
/// regex API a caller is likely to have used. An unparticipating group is an
/// empty string.
[[nodiscard]] inline std::vector<std::string> find(std::string_view text, std::string_view expression,
                                                   bool ignore_case = false) {
    std::vector<std::string> groups;
    auto compiled = pattern(expression, ignore_case);
    if (!compiled) return groups;

    const std::string subject{text};
    std::smatch       match;
    if (!std::regex_search(subject, match, *compiled)) return groups;
    groups.reserve(match.size());
    for (const auto& group : match) groups.push_back(group.matched ? group.str() : std::string{});
    return groups;
}

/// Every match of the pattern, whole matches only.
[[nodiscard]] inline std::vector<std::string> find_all(std::string_view text,
                                                       std::string_view expression,
                                                       bool ignore_case = false) {
    std::vector<std::string> found;
    auto compiled = pattern(expression, ignore_case);
    if (!compiled) return found;

    const std::string subject{text};
    for (auto it = std::sregex_iterator(subject.begin(), subject.end(), *compiled);
         it != std::sregex_iterator(); ++it) {
        found.push_back(it->str());
    }
    return found;
}

/// Replace every match. `$1`, `$2` and friends refer to capture groups.
/// Returns the input unchanged when the pattern does not compile.
[[nodiscard]] inline std::string replace(std::string_view text, std::string_view expression,
                                         std::string_view replacement, bool ignore_case = false) {
    auto compiled = pattern(expression, ignore_case);
    if (!compiled) return std::string{text};
    return std::regex_replace(std::string{text}, *compiled, std::string{replacement});
}

/// Split on a pattern. Empty fields are kept, so a caller can tell `a,,b` from
/// `a,b` — dropping them is a policy the caller should choose, not the split.
[[nodiscard]] inline std::vector<std::string> split(std::string_view text, std::string_view expression,
                                                    bool ignore_case = false) {
    std::vector<std::string> parts;
    auto compiled = pattern(expression, ignore_case);
    if (!compiled) {
        parts.emplace_back(text);
        return parts;
    }
    const std::string subject{text};
    std::sregex_token_iterator it(subject.begin(), subject.end(), *compiled, -1);
    for (const std::sregex_token_iterator end; it != end; ++it) parts.push_back(*it);
    return parts;
}

/// Trim ASCII whitespace from both ends.
[[nodiscard]] inline std::string trim(std::string_view text) {
    const auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    std::size_t first = 0;
    while (first < text.size() && is_space(static_cast<unsigned char>(text[first]))) ++first;
    std::size_t last = text.size();
    while (last > first && is_space(static_cast<unsigned char>(text[last - 1]))) --last;
    return std::string{text.substr(first, last - first)};
}

// ---------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------

/// A file path with the operations you usually want on one.
///
/// Modelled on libGDX's FileHandle: a value wrapping a path, with readString
/// and writeString available both as instance methods and as statics, so a
/// one-off read is one call and a path used repeatedly is one object.
///
/// Nothing here throws. Reads return an empty optional on failure and writes
/// return false, because a terminal program's response to "the file is not
/// there" is nearly always to print something and carry on, not to unwind.
class FileHandle {
public:
    FileHandle() = default;

    /// One constructor, not three. std::filesystem::path already converts from
    /// std::string, std::string_view and const char*, so adding overloads for
    /// them only creates ambiguity: `FileHandle h{some_string}` would match
    /// two candidates equally well and fail to compile.
    explicit FileHandle(std::filesystem::path path) : path_(std::move(path)) {}

    // --- static convenience ------------------------------------------------

    /// Read a whole file. Empty optional if it cannot be read.
    [[nodiscard]] static std::optional<std::string> readString(const std::filesystem::path& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) return std::nullopt;
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (in.bad()) return std::nullopt;
        return content;
    }

    /// Write a whole file, creating parent directories as needed.
    [[nodiscard]] static bool writeString(const std::filesystem::path& path, std::string_view content,
                                          bool append = false) {
        std::error_code ec;
        if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path(), ec);

        auto mode = std::ios::binary | (append ? std::ios::app : std::ios::trunc);
        std::ofstream out(path, mode);
        if (!out) return false;
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        return static_cast<bool>(out);
    }

    // --- instance ----------------------------------------------------------

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    [[nodiscard]] std::string name() const { return path_.filename().string(); }
    [[nodiscard]] std::string extension() const { return path_.extension().string(); }
    [[nodiscard]] std::string nameWithoutExtension() const { return path_.stem().string(); }
    [[nodiscard]] FileHandle parent() const { return FileHandle{path_.parent_path()}; }

    /// Join a child path. Named `child` after libGDX; `operator/` would read
    /// as division to anyone who has not seen std::filesystem.
    [[nodiscard]] FileHandle child(std::string_view name) const {
        return FileHandle{path_ / std::filesystem::path{name}};
    }

    [[nodiscard]] bool exists() const {
        std::error_code ec;
        return std::filesystem::exists(path_, ec);
    }

    [[nodiscard]] bool isDirectory() const {
        std::error_code ec;
        return std::filesystem::is_directory(path_, ec);
    }

    [[nodiscard]] std::uintmax_t length() const {
        std::error_code ec;
        const auto size = std::filesystem::file_size(path_, ec);
        return ec ? 0 : size;
    }

    [[nodiscard]] std::optional<std::string> readString() const { return readString(path_); }

    [[nodiscard]] bool writeString(std::string_view content, bool append = false) const {
        return writeString(path_, content, append);
    }

    /// Read as lines, with newlines removed. A trailing newline does not
    /// produce a final empty line, which is what every line-oriented tool
    /// assumes.
    [[nodiscard]] std::vector<std::string> readLines() const {
        std::vector<std::string> lines;
        auto content = readString();
        if (!content) return lines;

        std::string current;
        for (char c : *content) {
            if (c == '\n') {
                if (!current.empty() && current.back() == '\r') current.pop_back();
                lines.push_back(std::move(current));
                current.clear();
            } else {
                current.push_back(c);
            }
        }
        if (!current.empty()) lines.push_back(std::move(current));
        return lines;
    }

    [[nodiscard]] bool mkdirs() const {
        std::error_code ec;
        std::filesystem::create_directories(path_, ec);
        return !ec || std::filesystem::is_directory(path_, ec);
    }

    [[nodiscard]] bool remove() const {
        std::error_code ec;
        return std::filesystem::remove(path_, ec) && !ec;
    }

    /// Immediate children, sorted. Sorted because directory enumeration order
    /// is not stable and a program that lists files should not reorder itself
    /// between runs.
    [[nodiscard]] std::vector<FileHandle> list() const {
        std::vector<FileHandle> entries;
        std::error_code         ec;
        for (std::filesystem::directory_iterator it(path_, ec), end; it != end; it.increment(ec)) {
            if (ec) break;
            entries.emplace_back(it->path());
        }
        std::sort(entries.begin(), entries.end(),
                  [](const FileHandle& a, const FileHandle& b) { return a.path_ < b.path_; });
        return entries;
    }

private:
    std::filesystem::path path_;
};

/// Stream a FileHandle by path, so println(handle) does something sensible.
inline std::ostream& operator<<(std::ostream& out, const FileHandle& handle) {
    return out << handle.path().string();
}

}  // namespace sq::term

#endif  // SQUARED_KIT_TERMINAL_HPP
