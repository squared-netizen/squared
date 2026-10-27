#pragma once

#include <squared/gui/src/alignment.hpp>
#include <squared/messaging/name.hpp>
#include <squared/messaging/src/value_type.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace sq::gui
{

/**
 * @brief What a layout node builds. A layout file names it as `type`
 * (`"check_box"`); C++ names it through the widget (`add<CheckBox>`).
 */
enum class NodeType : std::uint8_t {
    label,
    button,
    check_box,
    text_field,
    slider,
    progress_bar,
    image,
    separator,
    table,
    scroll,
    stack,
    custom,
};

/** @brief How many node types there are. */
inline constexpr std::size_t node_type_count = 12;

/** @brief A type's name in a layout file: `check_box`. */
[[nodiscard]] std::string_view node_type_name( NodeType type ) noexcept;

/** @brief The type a layout file names; nothing for a name that is not one. */
[[nodiscard]] std::optional<NodeType> node_type_named( std::string_view name ) noexcept;

/** @brief What a layout is pushed as: a whole screen, or a pane over one. */
enum class LayoutKind : std::uint8_t { screen, pane };

/**
 * @brief Where a pane sits on its screen. `center` at the size it asks for;
 * `left` and `right` the screen's height, `top` and `bottom` its width.
 * Always inside the safe area.
 */
enum class PanePlace : std::uint8_t { center, left, right, top, bottom };

/** @brief How many places there are. */
inline constexpr std::size_t pane_place_count = 5;

/** @brief A place's name in a layout file: `left`. */
[[nodiscard]] std::string_view pane_place_name( PanePlace place ) noexcept;

/** @brief The place a layout file names; nothing for a name that is not one. */
[[nodiscard]] std::optional<PanePlace> pane_place_named( std::string_view name ) noexcept;

/**
 * @brief How a pane shows: where, and whether it is modal (the screen dims
 * and takes no taps until it closes). A screen's is ignored.
 */
struct PaneShape final
{
        PanePlace place = PanePlace::center;
        bool modal = false;

        friend bool operator==( const PaneShape &, const PaneShape & ) noexcept = default;
};

/**
 * @brief One node of a layout: the widget it builds, where that sits in its
 * cell, what it shows and what a tap sends. 32 bytes, read-only once made.
 *
 * Nodes are in document order: a table's, scroll's or stack's nodes follow
 * it, and `inside` counts them, theirs included. Words are offsets into the
 * layout's text (`Layout::text`), 0 for none.
 */
struct LayoutNode final
{
        /// The address the node shows (and sets); none: 0.
        messaging::Name bind{};
        /// Its words; an image's drawable; a custom node's type.
        std::uint16_t text = 0;
        /// The line a tap sends (`read_tap`); buttons only.
        std::uint16_t tap = 0;
        /// `bind` as its dotted name, for the dev tools.
        std::uint16_t bind_text = 0;
        /// A style from the skin; none: `default`.
        std::uint16_t style = 0;
        /// How many nodes follow inside this one, at every depth.
        std::uint16_t inside = 0;
        /// Where its numbers start in `Layout::numbers` (`node_numbers` says how many).
        std::uint16_t numbers = 0;
        NodeType type = NodeType::label;
        Alignment align_x = Alignment::center;
        Alignment align_y = Alignment::center;
        /// The columns its cell takes.
        std::uint8_t span = 1;
        /// Space around it in its cell; below 0: none.
        float pad = -1.0F;
        bool grow_x : 1 = false;
        bool grow_y : 1 = false;
        bool fill_x : 1 = false;
        bool fill_y : 1 = false;
        /// Its cell starts the table's next row.
        bool new_row : 1 = false;
        /// A separator that runs up and down.
        bool vertical : 1 = false;
        /// It has its own numbers: a slider's minimum, maximum and step, a
        /// progress bar's minimum and maximum. Without: 0, 1 and no step.
        bool has_numbers : 1 = false;
        /// An edit sends `set` to `bind`: a check box, slider or text field
        /// whose address carries `set`.
        bool sends_set : 1 = false;
        /// The type of the value `bind` shows (its key's, or names.json's); none without one.
        messaging::ValueType value = messaging::ValueType::none;

        friend bool operator==( const LayoutNode &, const LayoutNode & ) noexcept = default;
};

static_assert( sizeof( LayoutNode ) == 32, "LayoutNode: see docs/developer/layouts.md" );

/** @brief Whether nodes go inside a node of this type: a table, a scroll (one) or a stack. */
[[nodiscard]] constexpr bool holds_nodes( NodeType type ) noexcept
{
    return type == NodeType::table || type == NodeType::scroll || type == NodeType::stack;
}

/** @brief Whether a node of this type shows a value an address gives it (`bind`). */
[[nodiscard]] constexpr bool shows_value( NodeType type ) noexcept
{
    switch( type ) {
        case NodeType::label:
        case NodeType::check_box:
        case NodeType::text_field:
        case NodeType::slider:
        case NodeType::progress_bar:
            return true;
        default:
            return false;
    }
}

/** @brief Whether a node of this type edits the value it shows: a check box, slider or text field. */
[[nodiscard]] constexpr bool edits_value( NodeType type ) noexcept
{
    return type == NodeType::check_box || type == NodeType::slider || type == NodeType::text_field;
}

/**
 * @brief Whether a node of this type can show a value of this type: a label
 * any but uids, a check box a bool, a text field text, a slider or progress
 * bar a number or an integer.
 */
[[nodiscard]] constexpr bool shows_type( NodeType type, messaging::ValueType value ) noexcept
{
    using messaging::ValueType;
    switch( type ) {
        case NodeType::label:
            return value != ValueType::none && value != ValueType::uids;
        case NodeType::check_box:
            return value == ValueType::boolean;
        case NodeType::text_field:
            return value == ValueType::text;
        case NodeType::slider:
        case NodeType::progress_bar:
            return value == ValueType::integer || value == ValueType::number;
        default:
            return false;
    }
}

/** @brief How many numbers a node of this type takes: a slider 3, a progress bar 2. */
[[nodiscard]] constexpr std::size_t node_numbers( NodeType type ) noexcept
{
    switch( type ) {
        case NodeType::slider:
            return 3;
        case NodeType::progress_bar:
            return 2;
        default:
            return 0;
    }
}

/**
 * @brief A screen or pane as data: its nodes, their words and numbers. A
 * view: it owns nothing, so what it points at outlives it. A compiled layout
 * points into the binary's read-only data; a C++ screen's points into
 * scratch memory (`LayoutWriter`).
 *
 * The text is every word one after another, each ended by '\0', and starts
 * with a '\0' of its own, so offset 0 is "none".
 */
class Layout final
{
    public:
        using Nodes = std::span<const LayoutNode>;

        constexpr Layout() noexcept = default;
        constexpr Layout( std::string_view name, LayoutKind kind, std::span<const LayoutNode> nodes,
                          std::string_view text, std::span<const float> numbers = {},
                          PaneShape pane = {} ) noexcept
            : name_( name )
            , nodes_( nodes )
            , text_( text )
            , numbers_( numbers )
            , kind_( kind )
            , pane_( pane )
        {}

        /** @brief Its name: what `push` and a saved screen stack call it. */
        [[nodiscard]] constexpr std::string_view name() const noexcept { return name_; }
        [[nodiscard]] constexpr LayoutKind kind() const noexcept { return kind_; }
        /** @brief How it shows when it is a pane: its file's `place` and `modal`. */
        [[nodiscard]] constexpr PaneShape pane() const noexcept { return pane_; }
        [[nodiscard]] constexpr Nodes nodes() const noexcept { return nodes_; }
        [[nodiscard]] constexpr std::span<const float> numbers() const noexcept { return numbers_; }

        /** @brief All of its text, as stored. */
        [[nodiscard]] constexpr std::string_view text() const noexcept { return text_; }

        /** @brief The words at `offset`; empty for 0, or outside the text. */
        [[nodiscard]] constexpr std::string_view text( std::uint16_t offset ) const noexcept
        {
            if( offset == 0 || offset >= text_.size() ) {
                return {};
            }
            const std::string_view rest = text_.substr( offset );
            return rest.substr( 0, rest.find( '\0' ) );
        }

    private:
        std::string_view name_;
        std::span<const LayoutNode> nodes_;
        std::string_view text_;
        std::span<const float> numbers_;
        LayoutKind kind_ = LayoutKind::screen;
        PaneShape pane_{}; // in the padding: a layout stays 72 bytes
};

/** @brief What a tap's line does. */
enum class TapKind : std::uint8_t {
    none,     ///< no line
    telegram, ///< `set ...` or `do ...`: sent to the model as it reads
    push,     ///< `push <name>`: that screen over this one
    replace,  ///< `replace <name>`: that screen instead of this one
    back,     ///< `back`: what the back button does
    open,     ///< `open <name>`: that pane over this screen
    close,    ///< `close`: the pane the tap is in
    quit,     ///< `quit`: the app
    invalid,  ///< none of these
};

/** @brief A tap's line, read: what it does, and to what. */
struct Tap final
{
        TapKind kind = TapKind::none;
        /** @brief The layout `push`, `replace` and `open` name; a telegram's whole line. */
        std::string_view target{};
};

/**
 * @brief Read a tap's line. A telegram is checked here only for its verb,
 * `set` or `do`; the build checks the rest against `names.json`, and
 * `messaging::read_telegram` reads it when it is tapped.
 */
[[nodiscard]] Tap read_tap( std::string_view line ) noexcept;

/** @brief The word a navigation starts with (`push`); empty for `none`, `telegram`, `invalid`. */
[[nodiscard]] std::string_view tap_word( TapKind kind ) noexcept;

/** @brief Containers nest at most this deep. */
inline constexpr std::size_t layout_depth = 16;

/** @brief What is wrong with a layout, if anything. */
enum class LayoutProblem : std::uint8_t {
    none,
    /// The text does not start and end with '\0', or an offset is outside it.
    bad_text,
    /// A node's `inside` runs past its container's end, or the layout's.
    bad_tree,
    /// Containers nested deeper than `layout_depth`.
    too_deep,
    /// Nodes inside one that is not a table, scroll or stack.
    not_a_container,
    /// A scroll with more than one node directly inside it.
    scroll_holds_one,
    /// `new_row` on a node that is not in a table.
    misplaced_row,
    /// A type that is not a `NodeType`.
    bad_type,
    /// A bind on a node that shows no value, a name its text does not hash
    /// to, a value type the node cannot show, or `sends_set` on a node that
    /// does not edit what it is bound to.
    bad_bind,
    /// A tap on a node that is not a button, or a line `read_tap` refuses.
    bad_tap,
    /// Numbers outside the layout's, on a node that takes none, or a minimum
    /// not below its maximum.
    bad_numbers,
    /// A custom node without the name of its type.
    no_custom_name,
    // Found while building; everything else is still built:
    /// No custom type of that name: a label says so in its place.
    unknown_custom,
    /// An image's drawable is not in the skin: it shows nothing.
    unknown_drawable,
};

/** @brief What `check` or `instantiate` found: the first problem, and its node. */
struct LayoutCheck final
{
        LayoutProblem problem = LayoutProblem::none;
        std::uint16_t node = 0;

        [[nodiscard]] constexpr bool ok() const noexcept { return problem == LayoutProblem::none; }

        friend bool operator==( const LayoutCheck &, const LayoutCheck & ) noexcept = default;
};

/**
 * @brief Whether a layout can be built: its tree, text and numbers hold
 * together, and every node takes what it is given. `instantiate` checks first,
 * so a bad layout builds nothing.
 */
[[nodiscard]] LayoutCheck check( const Layout &layout ) noexcept;

/** @brief A problem in words, for a log line. */
[[nodiscard]] std::string_view problem_text( LayoutProblem problem ) noexcept;

} // namespace sq::gui
