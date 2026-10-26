#include <squared/data/json.hpp>
#include <squared/gui/layout_file.hpp>
#include <squared/messaging/telegram.hpp>
#include <squared/messaging/telegram_data.hpp>
#include <squared/messaging/telegram_text.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sq::gui
{

namespace detail
{

struct FileWriting
{
        static NodeWriter add( LayoutWriter &out, NodeWriter *parent, NodeType type,
                               std::string_view text ) noexcept
        {
            const std::uint16_t into = parent == nullptr ? LayoutWriter::root : parent->index_;
            return out.add_to( into, type, text );
        }

        static void bind( NodeWriter &node, messaging::Name name, std::string_view text ) noexcept
        {
            static_cast<void>( node.bind_name( name, text ) );
        }
};

} // namespace detail

namespace
{

using data::JsonValue;
using messaging::ValueType;
using messaging::Verb;

/// One file being read: where its problems are written, and whether any were.
class Reader final
{
    public:
        Reader( std::string_view file, FileProblems &problems ) noexcept
            : file_( file )
            , problems_( problems )
        {}

        void problem( const std::string &path, std::string what )
        {
            ok_ = false;
            std::string where( file_ );
            if( !path.empty() ) {
                where += ": " + path;
            }
            problems_.push_back( { std::move( where ), std::move( what ) } );
        }

        [[nodiscard]] bool ok() const noexcept { return ok_; }

    private:
        std::string_view file_;
        FileProblems &problems_;
        bool ok_ = true;
};

[[nodiscard]] std::string at( const std::string &path, std::size_t index )
{
    return path + "[" + std::to_string( index ) + "]";
}

[[nodiscard]] std::string field( const std::string &path, std::string_view key )
{
    std::string joined = path;
    if( !joined.empty() ) {
        joined += '.';
    }
    joined += key;
    return joined;
}

[[nodiscard]] std::optional<double> number_of( const JsonValue &value ) noexcept
{
    if( const std::int64_t *whole = value.signed_integer_if() ) {
        return static_cast<double>( *whole );
    }
    if( const std::uint64_t *whole = value.unsigned_integer_if() ) {
        return static_cast<double>( *whole );
    }
    if( const double *real = value.real_if() ) {
        return *real;
    }
    return std::nullopt;
}

/// A whole number, stored signed or not (the reader keeps a positive one unsigned).
[[nodiscard]] std::optional<std::int64_t> whole_of( const JsonValue &value ) noexcept
{
    if( const std::int64_t *signed_whole = value.signed_integer_if() ) {
        return *signed_whole;
    }
    const std::uint64_t *whole = value.unsigned_integer_if();
    if( whole != nullptr && *whole <= INT64_MAX ) {
        return static_cast<std::int64_t>( *whole );
    }
    return std::nullopt;
}

[[nodiscard]] std::string_view type_words( ValueType type ) noexcept
{
    switch( type ) {
        case ValueType::none:
            return "no value";
        case ValueType::boolean:
            return "a bool";
        case ValueType::integer:
            return "an int";
        case ValueType::number:
            return "a number";
        case ValueType::vec2:
            return "a vec2";
        case ValueType::vec3:
            return "a vec3";
        case ValueType::vec4:
            return "a vec4";
        case ValueType::text:
            return "text";
        case ValueType::uids:
            return "uids";
    }
    return "a value";
}

[[nodiscard]] std::optional<ValueType> value_type_named( std::string_view word ) noexcept
{
    constexpr std::array<std::pair<std::string_view, ValueType>, 8> types = { {
        { "bool", ValueType::boolean },
        { "int", ValueType::integer },
        { "number", ValueType::number },
        { "vec2", ValueType::vec2 },
        { "vec3", ValueType::vec3 },
        { "vec4", ValueType::vec4 },
        { "text", ValueType::text },
        { "uids", ValueType::uids },
    } };
    for( const auto &[name, type] : types ) {
        if( name == word ) {
            return type;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<Verb> verb_named( std::string_view word ) noexcept
{
    for( const Verb verb : { Verb::set, Verb::do_, Verb::state, Verb::event } ) {
        if( messaging::verb_text( verb ) == word ) {
            return verb;
        }
    }
    return std::nullopt;
}

[[nodiscard]] messaging::Verbs verbs_of( Verb verb ) noexcept
{
    return messaging::Verbs{ static_cast<std::uint8_t>( 1U << static_cast<unsigned>( verb ) ) };
}

// ---- names.json -------------------------------------------------------------

[[nodiscard]] NameEntry entry_of( const std::string &name, ValueType type, messaging::Verbs verbs,
                                  bool uid )
{
    NameEntry entry;
    entry.name = name;
    entry.type = type;
    entry.verbs = verbs;
    entry.uid = uid;
    return entry;
}

void read_list( Reader &reader, const std::string &path, const std::string &name,
                const JsonValue::Object &fields, std::vector<NameEntry> &out )
{
    NameEntry rows = entry_of( name, ValueType::integer, messaging::state, true );
    for( const auto &[key, value] : fields ) {
        if( key == "list" ) {
            const std::string *element = value.string_if();
            if( element == nullptr || !messaging::valid_name( *element ) ) {
                reader.problem( field( path, key ), "a list names its rows' element: \"item\"" );
            } else {
                rows.list = *element;
            }
        } else if( key == "note" ) {
            if( const std::string *note = value.string_if() ) {
                rows.note = *note;
            }
        } else {
            reader.problem( field( path, key ), "not a key of a list (list, note)" );
        }
    }
    out.push_back( rows );
    NameEntry count = entry_of( name + ".count", ValueType::integer, messaging::state, false );
    count.note = "How many rows " + name + " has";
    out.push_back( std::move( count ) );
}

void read_verbs( Reader &reader, const std::string &path, const JsonValue &value, NameEntry &entry )
{
    const JsonValue::Array *words = value.array_if();
    if( words == nullptr || words->empty() ) {
        reader.problem( path, "a list of verbs: state, event, set, do" );
        return;
    }
    for( std::size_t i = 0; i < words->size(); ++i ) {
        const std::string *word = ( *words )[i].string_if();
        std::optional<Verb> verb;
        if( word != nullptr ) {
            verb = verb_named( *word );
        }
        if( !verb ) {
            reader.problem( at( path, i ), "not a verb: state, event, set or do" );
        } else if( entry.verbs.has( *verb ) ) {
            reader.problem( at( path, i ), "given twice" );
        } else {
            entry.verbs = entry.verbs | verbs_of( *verb );
        }
    }
}

void read_entry( Reader &reader, const std::string &path, const std::string &name,
                 const JsonValue::Object &fields, std::vector<NameEntry> &out )
{
    if( fields.contains( "list" ) ) {
        read_list( reader, path, name, fields, out );
        return;
    }
    NameEntry entry = entry_of( name, ValueType::none, {}, false );
    bool has_verbs = false;
    const bool typed = fields.contains( "type" );
    for( const auto &[key, value] : fields ) {
        const std::string where = field( path, key );
        if( key == "type" ) {
            const std::string *word = value.string_if();
            std::optional<ValueType> type;
            if( word != nullptr ) {
                type = value_type_named( *word );
            }
            if( !type ) {
                reader.problem( where,
                                "not a type: bool, int, number, vec2, vec3, vec4, text, uids" );
            } else {
                entry.type = *type;
            }
        } else if( key == "verbs" ) {
            has_verbs = true;
            read_verbs( reader, where, value, entry );
        } else if( key == "uid" ) {
            const bool *uid = value.boolean_if();
            if( uid == nullptr ) {
                reader.problem( where, "true or false" );
            } else {
                entry.uid = *uid;
            }
        } else if( key == "note" ) {
            if( const std::string *note = value.string_if() ) {
                entry.note = *note;
            } else {
                reader.problem( where, "words, in a string" );
            }
        } else {
            reader.problem( where, "not a key of an address (type, verbs, uid, note; or list)" );
        }
    }
    if( !has_verbs ) {
        reader.problem( path, "no verbs: say which of state, event, set and do it carries" );
    }
    const bool valued = entry.verbs.has( Verb::state ) || entry.verbs.has( Verb::set );
    if( valued && !typed ) {
        reader.problem( path, "state and set carry a value: give it a type" );
    }
    out.push_back( std::move( entry ) );
}

void read_screens( Reader &reader, const JsonValue &value, std::vector<std::string> &out )
{
    const JsonValue::Array *names = value.array_if();
    if( names == nullptr ) {
        reader.problem( "screens", "a list of the C++ screens and panes layouts may open" );
        return;
    }
    for( std::size_t i = 0; i < names->size(); ++i ) {
        const std::string *name = ( *names )[i].string_if();
        if( name == nullptr || !messaging::valid_name( *name ) ) {
            reader.problem( at( "screens", i ), "a screen's name: lowercase words" );
        } else if( std::find( out.begin(), out.end(), *name ) != out.end() ) {
            reader.problem( at( "screens", i ), *name + " given twice" );
        } else {
            out.push_back( *name );
        }
    }
}

// ---- layouts ----------------------------------------------------------------

/// Which keys a node of a type takes, beyond `type` and `note`.
[[nodiscard]] bool takes_key( NodeType type, std::string_view key ) noexcept
{
    constexpr std::array<std::string_view, 9> cell = {
        "grow", "grow_x", "grow_y", "fill", "fill_x", "fill_y", "pad", "align", "span",
    };
    if( std::find( cell.begin(), cell.end(), key ) != cell.end() ) {
        return true;
    }
    switch( type ) {
        case NodeType::label:
        case NodeType::check_box:
        case NodeType::text_field:
            return key == "text" || key == "bind" || key == "style";
        case NodeType::button:
            return key == "text" || key == "tap" || key == "style";
        case NodeType::slider:
            return key == "bind" || key == "style" || key == "min" || key == "max" || key == "step";
        case NodeType::progress_bar:
            return key == "bind" || key == "style" || key == "min" || key == "max";
        case NodeType::image:
            return key == "text";
        case NodeType::separator:
            return key == "vertical";
        case NodeType::table:
            return key == "rows";
        case NodeType::scroll:
            return key == "node";
        case NodeType::stack:
            return key == "nodes";
        case NodeType::custom:
            return key == "name";
    }
    return false;
}

[[nodiscard]] bool shows( NodeType node, ValueType value ) noexcept
{
    switch( node ) {
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

[[nodiscard]] std::optional<Alignment> alignment_named( const JsonValue &value ) noexcept
{
    const std::string *word = value.string_if();
    if( word == nullptr ) {
        return std::nullopt;
    }
    if( *word == "start" ) {
        return Alignment::start;
    }
    if( *word == "center" ) {
        return Alignment::center;
    }
    if( *word == "end" ) {
        return Alignment::end;
    }
    return std::nullopt;
}

/// Every node type's name, for a message.
[[nodiscard]] std::string type_list()
{
    std::string list;
    for( std::size_t i = 0; i < node_type_count; ++i ) {
        list += ( i == 0 ? "" : ", " );
        list += node_type_name( static_cast<NodeType>( i ) );
    }
    return list;
}

[[nodiscard]] bool by_name( const NameEntry &a, const NameEntry &b ) noexcept
{
    return a.name < b.name;
}

[[nodiscard]] bool name_below( const NameEntry &entry, std::string_view wanted ) noexcept
{
    return entry.name < wanted;
}

/// The dotted name a telegram's line addresses: its second word, up to a uid.
[[nodiscard]] std::string_view telegram_name( std::string_view line ) noexcept
{
    const std::size_t start = line.find( ' ' ) + 1;
    const std::size_t end = line.find_first_of( " [", start );
    return line.substr( start, end == std::string_view::npos ? end : end - start );
}

/// One layout file being read into a writer.
class LayoutReader final
{
    public:
        LayoutReader( Reader &reader, const NameSchema &schema,
                      std::span<const LayoutTarget> targets, LayoutKind kind,
                      LayoutWriter &out ) noexcept
            : reader_( reader )
            , schema_( schema )
            , targets_( targets )
            , kind_( kind )
            , out_( out )
        {}

        void rows( const std::string &path, const JsonValue &value, NodeWriter *parent )
        {
            const JsonValue::Array *rows = value.array_if();
            if( rows == nullptr ) {
                reader_.problem( path, "a list of rows, each a list of nodes" );
                return;
            }
            for( std::size_t i = 0; i < rows->size(); ++i ) {
                const JsonValue::Array *cells = ( *rows )[i].array_if();
                if( cells == nullptr || cells->empty() ) {
                    reader_.problem( at( path, i ), "a row is a list of one node or more" );
                    continue;
                }
                if( i > 0 && parent == nullptr ) {
                    out_.row();
                } else if( i > 0 ) {
                    parent->row();
                }
                for( std::size_t j = 0; j < cells->size(); ++j ) {
                    node( at( at( path, i ), j ), ( *cells )[j], parent );
                }
            }
        }

    private:
        void node( const std::string &path, const JsonValue &value, NodeWriter *parent )
        {
            const JsonValue::Object *fields = value.object_if();
            const JsonValue *type_value = value.find( "type" );
            const std::string *type_word = nullptr;
            if( type_value != nullptr ) {
                type_word = type_value->string_if();
            }
            if( fields == nullptr || type_word == nullptr ) {
                reader_.problem( path, "a node is an object with a type" );
                return;
            }
            const std::optional<NodeType> type = node_type_named( *type_word );
            if( !type ) {
                reader_.problem( field( path, "type" ), "not a type: " + type_list() );
                return;
            }
            for( const auto &[key, ignored] : *fields ) {
                if( key != "type" && key != "note" && !takes_key( *type, key ) ) {
                    reader_.problem( field( path, key ),
                                     "not a key of a " + std::string( *type_word ) );
                }
            }
            const std::string text = words( path, value, *type );
            NodeWriter written = detail::FileWriting::add( out_, parent, *type, text );
            cell( path, value, written );
            looks( path, value, *type, written );
            if( const JsonValue *bind = value.find( "bind" ) ) {
                this->bind( field( path, "bind" ), *bind, *type, written );
            }
            if( const JsonValue *tap = value.find( "tap" ) ) {
                this->tap( field( path, "tap" ), *tap, written );
            }
            inside( path, value, *type, written );
        }

        /// The node's words: `text`, or a custom node's `name`.
        [[nodiscard]] std::string words( const std::string &path, const JsonValue &value,
                                         NodeType type )
        {
            const std::string_view key = type == NodeType::custom ? "name" : "text";
            const JsonValue *found = value.find( key );
            if( found == nullptr ) {
                if( type == NodeType::custom ) {
                    reader_.problem( path, "a custom node names its type: \"name\": \"minimap\"" );
                } else if( type == NodeType::image ) {
                    reader_.problem( path, "an image names its drawable in text" );
                }
                return {};
            }
            const std::string *text = found->string_if();
            if( text == nullptr ) {
                reader_.problem( field( path, key ), "words, in a string" );
                return {};
            }
            return *text;
        }

        void cell( const std::string &path, const JsonValue &value, NodeWriter &node )
        {
            constexpr std::array<std::string_view, 6> flags = {
                "grow", "grow_x", "grow_y", "fill", "fill_x", "fill_y",
            };
            for( std::size_t i = 0; i < flags.size(); ++i ) {
                const JsonValue *flag = value.find( flags[i] );
                if( flag == nullptr ) {
                    continue;
                }
                const bool *on = flag->boolean_if();
                if( on == nullptr ) {
                    reader_.problem( field( path, flags[i] ), "true or false" );
                } else if( *on ) {
                    set_flag( node, i );
                }
            }
            if( const JsonValue *pad = value.find( "pad" ) ) {
                const std::optional<double> space = number_of( *pad );
                if( !space || !std::isfinite( *space ) || *space < 0.0 ) {
                    reader_.problem( field( path, "pad" ), "a number, 0 or more" );
                } else {
                    node.pad( static_cast<float>( *space ) );
                }
            }
            if( const JsonValue *span = value.find( "span" ) ) {
                const std::optional<std::int64_t> columns = whole_of( *span );
                if( !columns || *columns < 1 || *columns > 255 ) {
                    reader_.problem( field( path, "span" ), "a whole number of columns, 1 to 255" );
                } else {
                    node.span( static_cast<std::uint8_t>( *columns ) );
                }
            }
            if( const JsonValue *align = value.find( "align" ) ) {
                this->align( field( path, "align" ), *align, node );
            }
        }

        static void set_flag( NodeWriter &node, std::size_t which ) noexcept
        {
            switch( which ) {
                case 0:
                    node.grow();
                    break;
                case 1:
                    node.grow_x();
                    break;
                case 2:
                    node.grow_y();
                    break;
                case 3:
                    node.fill();
                    break;
                case 4:
                    node.fill_x();
                    break;
                default:
                    node.fill_y();
                    break;
            }
        }

        void align( const std::string &path, const JsonValue &value, NodeWriter &node )
        {
            const JsonValue::Array *both = value.array_if();
            if( both == nullptr ) {
                const std::optional<Alignment> one = alignment_named( value );
                if( !one ) {
                    reader_.problem( path, "start, center or end, or two of them: across, down" );
                    return;
                }
                node.align( *one ); // across; down stays centred, as NodeWriter::align( x ) does
                return;
            }
            std::optional<Alignment> across;
            std::optional<Alignment> down;
            if( both->size() == 2 ) {
                across = alignment_named( ( *both )[0] );
                down = alignment_named( ( *both )[1] );
            }
            if( !across || !down ) {
                reader_.problem( path, "two of start, center and end: across, then down" );
                return;
            }
            node.align( *across, *down );
        }

        void looks( const std::string &path, const JsonValue &value, NodeType type,
                    NodeWriter &node )
        {
            if( const JsonValue *style = value.find( "style" ) ) {
                const std::string *name = style->string_if();
                if( name == nullptr || name->empty() ) {
                    reader_.problem( field( path, "style" ), "a style's name from the skin" );
                } else {
                    node.style( *name );
                }
            }
            if( const JsonValue *vertical = value.find( "vertical" ) ) {
                const bool *on = vertical->boolean_if();
                if( on == nullptr ) {
                    reader_.problem( field( path, "vertical" ), "true or false" );
                } else if( *on ) {
                    node.vertical();
                }
            }
            numbers( path, value, type, node );
        }

        void numbers( const std::string &path, const JsonValue &value, NodeType type,
                      NodeWriter &node )
        {
            const JsonValue *min = value.find( "min" );
            const JsonValue *max = value.find( "max" );
            const JsonValue *step = value.find( "step" );
            if( min == nullptr && max == nullptr && step == nullptr ) {
                return;
            }
            const std::optional<double> low = number_or( path, min, "min", 0.0 );
            const std::optional<double> high = number_or( path, max, "max", 1.0 );
            const std::optional<double> by = number_or( path, step, "step", 0.0 );
            if( !low || !high || !by ) {
                return;
            }
            if( !( *low < *high ) ) {
                reader_.problem( path, "min is not below max" );
                return;
            }
            node.range( static_cast<float>( *low ), static_cast<float>( *high ) );
            if( step != nullptr && type == NodeType::slider ) {
                if( *by < 0.0 ) {
                    reader_.problem( field( path, "step" ), "a step of 0 or more" );
                    return;
                }
                node.step( static_cast<float>( *by ) );
            }
        }

        /// A number given at `key`, or `otherwise` when none is; nothing when it is not one.
        [[nodiscard]] std::optional<double> number_or( const std::string &path,
                                                       const JsonValue *given, std::string_view key,
                                                       double otherwise )
        {
            if( given == nullptr ) {
                return otherwise;
            }
            const std::optional<double> number = number_of( *given );
            if( !number || !std::isfinite( *number ) ) {
                reader_.problem( field( path, key ), "a number" );
                return std::nullopt;
            }
            return number;
        }

        void bind( const std::string &path, const JsonValue &value, NodeType type,
                   NodeWriter &node )
        {
            const std::string *name = value.string_if();
            const NameEntry *entry = name != nullptr ? schema_.find( *name ) : nullptr;
            if( name == nullptr ) {
                reader_.problem( path, "an address's name, from names.json" );
                return;
            }
            if( entry == nullptr ) {
                reader_.problem( path, *name + " is not in names.json" );
                return;
            }
            if( !entry->verbs.has( Verb::state ) ) {
                reader_.problem( path, *name + " carries no state to show" );
                return;
            }
            if( entry->uid ) {
                reader_.problem( path, *name + " takes a uid: only a list's row shows one "
                                               "(lists come in 7.5)" );
                return;
            }
            if( !shows( type, entry->type ) ) {
                std::string what = "a " + std::string( node_type_name( type ) );
                what += " cannot show " + std::string( type_words( entry->type ) );
                reader_.problem( path, what + " (" + *name + ")" );
                return;
            }
            detail::FileWriting::bind( node, messaging::Name::of( *name ), *name );
        }

        void tap( const std::string &path, const JsonValue &value, NodeWriter &node )
        {
            const std::string *line = value.string_if();
            const Tap read = line != nullptr ? read_tap( *line ) : Tap{ TapKind::invalid, {} };
            switch( read.kind ) {
                case TapKind::telegram:
                    if( !telegram( path, *line ) ) {
                        return;
                    }
                    break;
                case TapKind::push:
                case TapKind::replace:
                case TapKind::open:
                    if( !navigates( path, read ) ) {
                        return;
                    }
                    break;
                case TapKind::close:
                    if( kind_ != LayoutKind::pane ) {
                        reader_.problem( path, "close closes the pane it is in: this is a screen" );
                        return;
                    }
                    break;
                case TapKind::back:
                case TapKind::quit:
                    break;
                case TapKind::none:
                case TapKind::invalid:
                    reader_.problem( path, "not a tap: set or do (a telegram, as the dev log "
                                           "writes it), push, replace or open <name>, back, "
                                           "close, quit" );
                    return;
            }
            node.tap( *line );
        }

        [[nodiscard]] bool telegram( const std::string &path, const std::string &line )
        {
            std::array<std::byte, 4096> storage{};
            messaging::TelegramData data( storage );
            const std::optional<messaging::Telegram> read = messaging::read_telegram( line, data );
            const std::string name( telegram_name( line ) );
            if( !read ) {
                reader_.problem( path, "not a telegram as the dev log writes it: " + line );
                return false;
            }
            const NameEntry *entry = schema_.find_id( read->address().name );
            if( entry == nullptr ) {
                reader_.problem( path, name + " is not in names.json" );
                return false;
            }
            const std::string verb( messaging::verb_text( read->verb() ) );
            if( !entry->verbs.has( read->verb() ) ) {
                reader_.problem( path, name + " does not carry " + verb );
                return false;
            }
            if( entry->uid || read->address().uid != 0 ) {
                reader_.problem( path, name + " takes a uid: only a list's row sends one "
                                              "(lists come in 7.5)" );
                return false;
            }
            if( read->type() != entry->type ) {
                const std::string wanted( type_words( entry->type ) );
                std::string what = name + " takes " + wanted;
                if( read->type() == ValueType::none ) {
                    what += ": give it one";
                } else if( entry->type != ValueType::none ) {
                    what += ", not " + std::string( type_words( read->type() ) );
                }
                if( entry->type == ValueType::number && read->type() == ValueType::integer ) {
                    what += ": write a number with a point (7.0)";
                }
                reader_.problem( path, what );
                return false;
            }
            return true;
        }

        [[nodiscard]] bool navigates( const std::string &path, const Tap &read )
        {
            const std::string target( read.target );
            const bool to_pane = read.kind == TapKind::open;
            for( const LayoutTarget &layout : targets_ ) {
                if( layout.name != target ) {
                    continue;
                }
                if( to_pane && layout.kind != LayoutKind::pane ) {
                    reader_.problem( path, target + " is a screen: push or replace it" );
                    return false;
                }
                if( !to_pane && layout.kind != LayoutKind::screen ) {
                    reader_.problem( path, target + " is a pane: open it" );
                    return false;
                }
                return true;
            }
            if( schema_.has_screen( target ) ) {
                return true;
            }
            const std::string what = "no layout or screen is called " + target;
            reader_.problem( path, what + " (a C++ screen is listed in names.json's screens)" );
            return false;
        }

        void inside( const std::string &path, const JsonValue &value, NodeType type,
                     NodeWriter &node )
        {
            if( type == NodeType::table ) {
                if( const JsonValue *rows = value.find( "rows" ) ) {
                    this->rows( field( path, "rows" ), *rows, &node );
                }
            } else if( type == NodeType::scroll ) {
                if( const JsonValue *one = value.find( "node" ) ) {
                    this->node( field( path, "node" ), *one, &node );
                }
            } else if( type == NodeType::stack ) {
                if( const JsonValue *many = value.find( "nodes" ) ) {
                    const JsonValue::Array *list = many->array_if();
                    if( list == nullptr ) {
                        reader_.problem( field( path, "nodes" ), "a list of nodes" );
                        return;
                    }
                    for( std::size_t i = 0; i < list->size(); ++i ) {
                        this->node( at( field( path, "nodes" ), i ), ( *list )[i], &node );
                    }
                }
            }
        }

        Reader &reader_;
        const NameSchema &schema_;
        std::span<const LayoutTarget> targets_;
        LayoutKind kind_;
        LayoutWriter &out_;
};

} // namespace

const NameEntry *NameSchema::find( std::string_view name ) const noexcept
{
    const auto found = std::lower_bound( names.begin(), names.end(), name, name_below );
    return found != names.end() && found->name == name ? &*found : nullptr;
}

const NameEntry *NameSchema::find_id( messaging::Name name ) const noexcept
{
    for( const NameEntry &entry : names ) {
        if( messaging::Name::of( entry.name ) == name ) {
            return &entry;
        }
    }
    return nullptr;
}

bool NameSchema::has_screen( std::string_view name ) const noexcept
{
    return std::find( screens.begin(), screens.end(), name ) != screens.end();
}

bool read_json( std::string_view file, std::string_view text, data::JsonValue &out,
                FileProblems &problems )
{
    data::JsonParseResult parsed = data::parse_json( text );
    if( parsed ) {
        out = std::move( parsed.value );
        return true;
    }
    const data::JsonError &error = parsed.error;
    FileProblem problem;
    problem.where = std::string( file ) + ":" + std::to_string( error.line );
    problem.where += ":" + std::to_string( error.column );
    problem.what = "not JSON: " + error.message;
    problems.push_back( std::move( problem ) );
    return false;
}

bool read_names( std::string_view file, const data::JsonValue &root, NameSchema &schema,
                 FileProblems &problems )
{
    Reader reader( file, problems );
    const JsonValue::Object *top = root.object_if();
    if( top == nullptr ) {
        reader.problem( {},
                        "not an object: names.json is { \"names\": {...}, \"screens\": [...] }" );
        return false;
    }
    std::vector<NameEntry> entries;
    for( const auto &[key, value] : *top ) {
        if( key == "names" ) {
            const JsonValue::Object *names = value.object_if();
            if( names == nullptr ) {
                reader.problem( key, "an object: each address's name, and what it carries" );
                continue;
            }
            for( const auto &[name, spec] : *names ) {
                const std::string path = "names[\"" + name + "\"]";
                const JsonValue::Object *fields = spec.object_if();
                if( !messaging::valid_name( name ) ) {
                    reader.problem( path, "not a name: lowercase words, digits and _, "
                                          "between dots" );
                } else if( fields == nullptr ) {
                    reader.problem( path, "an object: type, verbs, uid, note; or list" );
                } else {
                    read_entry( reader, path, name, *fields, entries );
                }
            }
        } else if( key == "screens" ) {
            read_screens( reader, value, schema.screens );
        } else if( key != "note" ) {
            reader.problem( key, "not a key of names.json (names, screens, note)" );
        }
    }
    std::sort( entries.begin(), entries.end(), by_name );
    for( std::size_t i = 1; i < entries.size(); ++i ) {
        if( entries[i].name == entries[i - 1].name ) {
            reader.problem( "names[\"" + entries[i].name + "\"]",
                            "declared twice (a list declares <name>.count itself)" );
        }
    }
    // Ids are 64-bit hashes: two names with one id would be one address.
    std::vector<std::pair<std::uint64_t, std::string>> ids;
    ids.reserve( entries.size() );
    for( const NameEntry &entry : entries ) {
        ids.emplace_back( messaging::Name::of( entry.name ).id, entry.name );
    }
    std::sort( ids.begin(), ids.end() );
    for( std::size_t i = 1; i < ids.size(); ++i ) {
        if( ids[i].first == ids[i - 1].first && ids[i].second != ids[i - 1].second ) {
            reader.problem( "names[\"" + ids[i].second + "\"]",
                            "hashes to the same id as " + ids[i - 1].second + ": rename one" );
        }
    }
    schema.names = std::move( entries );
    return reader.ok();
}

LayoutKind read_layout_kind( std::string_view file, const data::JsonValue &root,
                             FileProblems &problems )
{
    Reader reader( file, problems );
    if( root.object_if() == nullptr ) {
        reader.problem( {}, "not an object: a layout is { \"kind\": ..., \"rows\": [...] }" );
        return LayoutKind::screen;
    }
    const JsonValue *kind = root.find( "kind" );
    if( kind == nullptr ) {
        return LayoutKind::screen;
    }
    const std::string *word = kind->string_if();
    if( word != nullptr && *word == "screen" ) {
        return LayoutKind::screen;
    }
    if( word != nullptr && *word == "pane" ) {
        return LayoutKind::pane;
    }
    reader.problem( "kind", "screen or pane" );
    return LayoutKind::screen;
}

namespace
{

/// The keys a layout file has: `place` and `modal` a pane's only.
[[nodiscard]] bool layout_key( std::string_view key ) noexcept
{
    constexpr std::array<std::string_view, 5> keys{ "kind", "rows", "note", "place", "modal" };
    return std::find( keys.begin(), keys.end(), key ) != keys.end();
}

/// A pane's `place` and `modal`; a screen has neither.
[[nodiscard]] PaneShape read_pane_shape( Reader &reader, const JsonValue &root, LayoutKind kind )
{
    PaneShape shape;
    const JsonValue *place = root.find( "place" );
    const JsonValue *modal = root.find( "modal" );
    if( kind != LayoutKind::pane ) {
        if( place != nullptr ) {
            reader.problem( "place", "a pane's: this is a screen (\"kind\": \"pane\")" );
        }
        if( modal != nullptr ) {
            reader.problem( "modal", "a pane's: this is a screen (\"kind\": \"pane\")" );
        }
        return shape;
    }
    if( place != nullptr ) {
        const std::string *word = place->string_if();
        std::optional<PanePlace> read;
        if( word != nullptr ) {
            read = pane_place_named( *word );
        }
        if( read ) {
            shape.place = *read;
        } else {
            reader.problem( "place", "center, left, right, top or bottom" );
        }
    }
    if( modal != nullptr ) {
        const bool *read = modal->boolean_if();
        if( read != nullptr ) {
            shape.modal = *read;
        } else {
            reader.problem( "modal", "true or false" );
        }
    }
    return shape;
}

} // namespace

bool read_layout( std::string_view file, const data::JsonValue &root, const NameSchema &schema,
                  std::span<const LayoutTarget> targets, LayoutWriter &out, FileProblems &problems )
{
    const std::size_t before = problems.size();
    const LayoutKind kind = read_layout_kind( file, root, problems );
    if( problems.size() != before ) {
        return false;
    }
    Reader reader( file, problems );
    for( const auto &[key, ignored] : *root.object_if() ) {
        if( !layout_key( key ) ) {
            reader.problem( key, "not a key of a layout (kind, rows, note; a pane: place, modal)" );
        }
    }
    out.set_pane( read_pane_shape( reader, root, kind ) );
    LayoutReader layout( reader, schema, targets, kind, out );
    if( const JsonValue *rows = root.find( "rows" ) ) {
        layout.rows( "rows", *rows, nullptr );
    }
    if( reader.ok() && !out.complete() ) {
        reader.problem( {}, "more than a layout's storage holds" );
    }
    return reader.ok();
}

} // namespace sq::gui
