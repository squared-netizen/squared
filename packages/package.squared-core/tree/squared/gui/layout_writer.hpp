#pragma once

#include <squared/gui/layout.hpp>
#include <squared/gui/src/alignment.hpp>
#include <squared/messaging/key.hpp>
#include <squared/messaging/name.hpp>
#include <squared/messaging/src/pack.hpp>
#include <squared/messaging/src/verb.hpp>
#include <squared/messaging/telegram.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace sq::gui
{

class Button;
class CheckBox;
class Image;
class Label;
class ProgressBar;
class ScrollPane;
class Separator;
class Slider;
class Stack;
class Table;
class TextField;

/** @brief The node type a widget is written as: `add<CheckBox>` writes a `check_box`. */
template<typename W>
struct NodeTypeOf; // not a widget a layout can hold: does not compile

template<>
struct NodeTypeOf<Label>
{
        static constexpr NodeType type = NodeType::label;
};
template<>
struct NodeTypeOf<Button>
{
        static constexpr NodeType type = NodeType::button;
};
template<>
struct NodeTypeOf<CheckBox>
{
        static constexpr NodeType type = NodeType::check_box;
};
template<>
struct NodeTypeOf<TextField>
{
        static constexpr NodeType type = NodeType::text_field;
};
template<>
struct NodeTypeOf<Slider>
{
        static constexpr NodeType type = NodeType::slider;
};
template<>
struct NodeTypeOf<ProgressBar>
{
        static constexpr NodeType type = NodeType::progress_bar;
};
template<>
struct NodeTypeOf<Image>
{
        static constexpr NodeType type = NodeType::image;
};
template<>
struct NodeTypeOf<Separator>
{
        static constexpr NodeType type = NodeType::separator;
};
template<>
struct NodeTypeOf<Table>
{
        static constexpr NodeType type = NodeType::table;
};
template<>
struct NodeTypeOf<ScrollPane>
{
        static constexpr NodeType type = NodeType::scroll;
};
template<>
struct NodeTypeOf<Stack>
{
        static constexpr NodeType type = NodeType::stack;
};

namespace detail
{

using messaging::Verb;

/// A key a node can show: it carries `state`, and takes no uid.
template<typename K>
concept shown_key = messaging::is_key<K> && K::verbs.has( Verb::state ) && !K::takes_uid;

/// A key a tap sends with no value: `do <key>`.
template<typename K>
concept done_key = messaging::sendable<K, Verb::do_, void> && !K::takes_uid;

template<typename K, typename V>
concept does = messaging::sendable<K, Verb::do_, V>;

template<typename K, typename V>
concept sets = messaging::sendable<K, Verb::set, V>;

/// A key and a value a tap sends: `do`, or `set` for a key without `do`.
template<typename K, typename V>
concept sent_value = ( does<K, V> || sets<K, V> ) && !K::takes_uid;

template<typename K>
[[nodiscard]] constexpr Verb tap_verb() noexcept
{
    return K::verbs.has( Verb::do_ ) ? Verb::do_ : Verb::set;
}

} // namespace detail

/** @brief Where a tap goes, when it navigates rather than sends. */
enum class Navigate : std::uint8_t { push, replace, back, open, close, quit };

class LayoutWriter;

/**
 * @brief One node being written: each call sets something about it and
 * returns it, so calls chain. A node that could not be written (the storage
 * is full, or it was added where it cannot go) takes the calls and keeps
 * nothing; `LayoutWriter::complete` says so.
 */
class NodeWriter final
{
    public:
        /** @brief Take the cell's spare room both ways, or one. */
        NodeWriter &grow() noexcept;
        NodeWriter &grow_x() noexcept;
        NodeWriter &grow_y() noexcept;
        /** @brief Fill the cell both ways, or one. */
        NodeWriter &fill() noexcept;
        NodeWriter &fill_x() noexcept;
        NodeWriter &fill_y() noexcept;
        /** @brief Space around it in its cell. */
        NodeWriter &pad( float space ) noexcept;
        NodeWriter &align( Alignment x, Alignment y = Alignment::center ) noexcept;
        /** @brief The columns its cell takes. */
        NodeWriter &span( std::uint8_t columns ) noexcept;
        /** @brief A style from the skin, in place of `default`. */
        NodeWriter &style( std::string_view name ) noexcept;
        /** @brief A separator that runs up and down. */
        NodeWriter &vertical() noexcept;
        /** @brief A slider's or progress bar's range; `minimum` below `maximum`. */
        NodeWriter &range( float minimum, float maximum ) noexcept;
        /** @brief A slider's step; 0: none. */
        NodeWriter &step( float step ) noexcept;

        /** @brief Show `key`'s `state` (a label, check box, text field, slider or progress bar). */
        template<detail::shown_key K>
        NodeWriter &bind( const K &key ) noexcept
        {
            return bind_name( key.name(), key.text() );
        }

        /** @brief A button's tap: a line as a layout file writes it (`do file.open "a.txt"`, `push settings`). */
        NodeWriter &tap( std::string_view line ) noexcept;

        /** @brief A button's tap that navigates; `target` names the screen or pane. */
        NodeWriter &tap( Navigate how, std::string_view target = {} ) noexcept;

        /** @brief A button's tap that sends `do <key>`. */
        template<detail::done_key K>
        NodeWriter &tap( const K &key ) noexcept
        {
            const messaging::Address to{ key.name(), 0 };
            const auto telegram = messaging::Telegram::empty( messaging::Verb::do_, to );
            return tap_telegram( telegram, {}, key.text() );
        }

        /** @brief A button's tap that sends `do <key> <value>`, or `set` for a key without `do`. */
        template<typename K, typename V>
            requires detail::sent_value<K, V>
        NodeWriter &tap( const K &key, const V &value ) noexcept
        {
            using T = typename K::value_type;
            const messaging::Address to{ key.name(), 0 };
            const auto packed = messaging::detail::pack<T>( detail::tap_verb<K>(), to, value );
            return tap_telegram( packed.telegram, packed.data, key.text() );
        }

        /** @brief Add a node inside this table, scroll (one only) or stack. */
        template<typename W>
        NodeWriter add( std::string_view text = {} ) noexcept;

        /** @brief Add a custom node inside this one: a type the app registers by `name`. */
        NodeWriter add_custom( std::string_view name ) noexcept;

        /** @brief In a table: the next node added to it starts a new row. */
        NodeWriter &row() noexcept;

        /** @brief Whether this node was written. */
        [[nodiscard]] bool written() const noexcept;

    private:
        friend class LayoutWriter;
        static constexpr std::uint16_t none = UINT16_MAX; ///< not written
        NodeWriter( LayoutWriter *writer, std::uint16_t index ) noexcept
            : writer_( writer )
            , index_( index )
        {}
        [[nodiscard]] LayoutNode *node() noexcept;
        NodeWriter &bind_name( messaging::Name name, std::string_view text ) noexcept;
        NodeWriter &tap_telegram( const messaging::Telegram &telegram,
                                  std::span<const std::byte> data, std::string_view name ) noexcept;
        [[nodiscard]] float *numbers() noexcept;

        LayoutWriter *writer_;
        std::uint16_t index_;
};

/**
 * @brief Writes a layout from C++, into storage the caller gives: what a C++
 * screen's `build()` does. The result is the same `Layout` a layout file
 * compiles to, so one instantiator builds both.
 *
 * ```cpp
 * root.add<sq::gui::CheckBox>( "Sound" ).bind( names::settings_sound );
 * root.row();
 * root.add<sq::gui::Button>( "Back" ).tap( sq::gui::Navigate::back );
 * ```
 *
 * Nodes go in document order, so a container takes nodes until a node is
 * added to one outside it; adding to it after that is a mistake (the node is
 * not written). Allocates nothing: words are copied into the text storage.
 */
class LayoutWriter final
{
    public:
        LayoutWriter( std::span<LayoutNode> nodes, std::span<char> text, std::span<float> numbers,
                      std::string_view name = {}, LayoutKind kind = LayoutKind::screen ) noexcept;
        LayoutWriter( const LayoutWriter & ) = delete;
        LayoutWriter &operator=( const LayoutWriter & ) = delete;
        LayoutWriter( LayoutWriter && ) = delete;
        LayoutWriter &operator=( LayoutWriter && ) = delete;
        ~LayoutWriter() = default;

        /** @brief Add a node to the layout's own table. */
        template<typename W>
        NodeWriter add( std::string_view text = {} ) noexcept
        {
            return add_to( root, NodeTypeOf<W>::type, text );
        }

        /** @brief Add a custom node: a type the app registers by `name`. */
        NodeWriter add_custom( std::string_view name ) noexcept;

        /** @brief The next node added to the layout's own table starts a new row. */
        LayoutWriter &row() noexcept;

        /** @brief What was written, as a layout. Points into the storage. */
        [[nodiscard]] Layout layout() const noexcept;

        /** @brief Everything asked for was written: nothing ran out of room or went where it cannot. */
        [[nodiscard]] bool complete() const noexcept { return complete_; }

        /** @brief Start again, with the same storage. */
        void clear() noexcept;

    private:
        friend class NodeWriter;

        /// The layout's own table, as a parent; a layout holds fewer nodes.
        static constexpr std::uint16_t root = UINT16_MAX - 1;

        /// A container still taking nodes.
        struct Open
        {
                std::uint16_t node;
                std::uint16_t children;
                bool row_next;
        };

        NodeWriter add_to( std::uint16_t parent, NodeType type, std::string_view text ) noexcept;
        [[nodiscard]] std::uint16_t put_text( std::string_view text ) noexcept;
        [[nodiscard]] Open *open_entry( std::uint16_t node ) noexcept;
        void refuse() noexcept { complete_ = false; }

        std::span<LayoutNode> nodes_;
        std::span<char> text_;
        std::span<float> numbers_;
        std::string_view name_;
        std::array<Open, layout_depth> open_{};
        std::size_t text_used_ = 0;
        std::size_t numbers_used_ = 0;
        std::uint16_t count_ = 0;
        std::uint16_t depth_ = 0;
        std::uint16_t root_children_ = 0;
        LayoutKind kind_;
        bool root_row_next_ = false;
        bool complete_ = true;
};

template<typename W>
NodeWriter NodeWriter::add( std::string_view text ) noexcept
{
    return writer_->add_to( index_, NodeTypeOf<W>::type, text );
}

} // namespace sq::gui
