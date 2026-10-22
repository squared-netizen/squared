#pragma once

#include <squared/data/json_value.hpp>
#include <squared/gui/layout.hpp>
#include <squared/gui/layout_writer.hpp>
#include <squared/messaging/name.hpp>
#include <squared/messaging/src/value_type.hpp>
#include <squared/messaging/src/verb.hpp>

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sq::gui
{

/**
 * @brief One address `names.json` declares: its dotted name, value type,
 * verbs, whether it takes a uid, and words for people.
 */
struct NameEntry final
{
        std::string name;
        messaging::ValueType type = messaging::ValueType::none;
        messaging::Verbs verbs{};
        bool uid = false;
        /// A list's element name (`"inventory": { "list": "item" }`); empty otherwise.
        std::string list;
        std::string note;
};

/**
 * @brief `names.json`, read: every address, sorted by name, and the C++
 * screens and panes a layout may navigate to.
 */
struct NameSchema final
{
        std::vector<NameEntry> names;
        std::vector<std::string> screens;

        [[nodiscard]] const NameEntry *find( std::string_view name ) const noexcept;
        /** @brief By its id: what a telegram read from text carries. */
        [[nodiscard]] const NameEntry *find_id( messaging::Name name ) const noexcept;
        [[nodiscard]] bool has_screen( std::string_view name ) const noexcept;
};

/**
 * @brief Something wrong in a file. `where` is the file and either a line
 * and column (a syntax error) or the path to the value inside it
 * (`settings.json: rows[1][0].bind`); `what` says what is wrong.
 */
struct FileProblem final
{
        std::string where;
        std::string what;
};

using FileProblems = std::vector<FileProblem>;

/** @brief A layout another one may navigate to: its name, and whether it is a screen or a pane. */
struct LayoutTarget final
{
        std::string name;
        LayoutKind kind = LayoutKind::screen;
};

/**
 * @brief Parse `text`, the JSON file `file`, into `out`. False, with the
 * problem added, when it is not JSON (its line and column).
 */
[[nodiscard]] bool read_json( std::string_view file, std::string_view text, data::JsonValue &out,
                              FileProblems &problems );

/**
 * @brief Read `names.json` into `schema`. A list (`{ "list": "item" }`) is
 * its rows' key (an integer `state` taking a uid) and `<name>.count`. False
 * when anything was wrong; each problem is added.
 */
[[nodiscard]] bool read_names( std::string_view file, const data::JsonValue &root,
                               NameSchema &schema, FileProblems &problems );

/**
 * @brief A layout file's kind (`"kind": "screen"` or `"pane"`; screen when
 * left out), read first: every file's kind is needed to check navigation.
 * When the kind is neither, or the file is not an object, the problem is
 * added and the kind given is screen.
 */
[[nodiscard]] LayoutKind read_layout_kind( std::string_view file, const data::JsonValue &root,
                                           FileProblems &problems );

/**
 * @brief Read a layout file into `out`, checking every node: its keys, their
 * values, binds against `schema` (the address exists, carries `state`, and has
 * a type the node can show), taps (a telegram whose address, verb and value
 * type the schema allows; a navigation to one of `targets` or a screen
 * `schema` lists). False when anything was wrong; each problem is added, and
 * what `out` holds is then not to be used.
 */
[[nodiscard]] bool read_layout( std::string_view file, const data::JsonValue &root,
                                const NameSchema &schema, std::span<const LayoutTarget> targets,
                                LayoutWriter &out, FileProblems &problems );

} // namespace sq::gui
