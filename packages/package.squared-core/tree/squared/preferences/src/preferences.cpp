module sq.preferences;

import std;
import sq.data;
import sq.files;

namespace sq::preferences
{

namespace
{

using files::FileType;

PreferencesError io_error( const files::FileError &error )
{
    return { PreferencesErrorCode::io, error.message.empty() ? error.path : error.message };
}

/** @brief `name` as a path under the tree, or empty when it would leave it. */
std::string tree_relative( std::string_view name )
{
    if( name.empty() || name.front() == '/' || name.find( '\\' ) != std::string_view::npos ) {
        return {};
    }
    std::string out;
    std::size_t start = 0;
    while( start <= name.size() ) {
        const std::size_t slash = std::min( name.find( '/', start ), name.size() );
        const std::string_view part = name.substr( start, slash - start );
        if( part.empty() || part == "." || part == ".." || part == "SQ-INF" ) {
            return {};
        }
        out += ( out.empty() ? "" : "/" ) + std::string( part );
        start = slash + 1;
    }
    return out;
}

std::string text_of( const std::vector<std::byte> &bytes )
{
    return { reinterpret_cast<const char *>( bytes.data() ), bytes.size() };
}

std::span<const std::byte> bytes_of( std::string_view text )
{
    return std::as_bytes( std::span( text.data(), text.size() ) );
}

} // namespace

Preferences::Preferences( files::FileSystem &files, std::string id, std::string folder,
                          data::JsonValue settings )
    : files_( &files )
    , id_( std::move( id ) )
    , folder_( std::move( folder ) )
    , settings_( std::move( settings ) )
{}

std::expected<Preferences, PreferencesError>
Preferences::open( files::FileSystem &files, std::string_view app_id, std::string_view folder )
{
    const std::string id = "preferences." + std::string( app_id );
    const std::string root( folder );
    const std::string manifest = root + "/SQ-INF/manifest.json";
    const std::string settings = root + "/tree/" + std::string( settings_file );

    if( !files.exists( FileType::Local, manifest ) ) {
        // Made, empty: the manifest sqcart 3.0 reads, and no settings.
        for( const std::string &dir : { root + "/SQ-INF", root + "/tree" } ) {
            if( const files::FileError e = files.make_directories( FileType::Local, dir ) ) {
                return std::unexpected( io_error( e ) );
            }
        }
        data::JsonValue::Object m;
        m.emplace( "format", data::JsonValue( "squared-cartridge" ) );
        m.emplace( "format_version", data::JsonValue( std::int64_t{ 3 } ) );
        m.emplace( "kind", data::JsonValue( kind ) );
        m.emplace( "id", data::JsonValue( id ) );
        m.emplace( "version", data::JsonValue( schema ) );
        m.emplace( "tree", data::JsonValue( "tree/" ) );
        m.emplace( "title", data::JsonValue( std::string( app_id ) + " preferences" ) );
        const data::JsonWriteResult text = data::write_json(
            data::JsonValue( std::move( m ) ), { .pretty = true, .newline_at_end = true } );
        if( const files::FileError e =
                files.write_atomically( FileType::Local, manifest, bytes_of( text.text ) ) ) {
            return std::unexpected( io_error( e ) );
        }
        return Preferences( files, id, root, data::JsonValue( data::JsonValue::Object{} ) );
    }

    const files::FileReadResult read = files.read( FileType::Local, manifest );
    if( !read ) {
        return std::unexpected( io_error( read.error ) );
    }
    const data::JsonParseResult parsed = data::parse_json( text_of( read.bytes ) );
    const data::JsonValue *k = parsed ? parsed.value.find( "kind" ) : nullptr;
    if( k == nullptr || k->string_if() == nullptr || *k->string_if() != kind ) {
        return std::unexpected(
            PreferencesError{ PreferencesErrorCode::not_preferences,
                              manifest + " is not a preferences cartridge's manifest" } );
    }
    std::string found = id;
    if( const data::JsonValue *i = parsed.value.find( "id" ); i != nullptr && i->string_if() ) {
        found = *i->string_if();
    }

    data::JsonValue values( data::JsonValue::Object{} );
    if( files.exists( FileType::Local, settings ) ) {
        const files::FileReadResult s = files.read( FileType::Local, settings );
        if( !s ) {
            return std::unexpected( io_error( s.error ) );
        }
        data::JsonParseResult p = data::parse_json( text_of( s.bytes ) );
        if( !p || p.value.object_if() == nullptr ) {
            return std::unexpected( PreferencesError{ PreferencesErrorCode::invalid_settings,
                                                      settings + " is not one JSON object" } );
        }
        values = std::move( p.value );
    }
    return Preferences( files, std::move( found ), root, std::move( values ) );
}

// ---- settings -------------------------------------------------------------------------------------

const data::JsonValue *Preferences::find( std::string_view key ) const noexcept
{
    return settings_.find( key );
}

bool Preferences::get_bool( std::string_view key, bool fallback ) const noexcept
{
    const data::JsonValue *v = find( key );
    const bool *b = v != nullptr ? v->boolean_if() : nullptr;
    return b != nullptr ? *b : fallback;
}

std::int64_t Preferences::get_integer( std::string_view key, std::int64_t fallback ) const noexcept
{
    const data::JsonValue *v = find( key );
    if( v == nullptr ) {
        return fallback;
    }
    if( const std::int64_t *i = v->signed_integer_if() ) {
        return *i;
    }
    if( const std::uint64_t *u = v->unsigned_integer_if() ) {
        return *u <= static_cast<std::uint64_t>( std::numeric_limits<std::int64_t>::max() )
                   ? static_cast<std::int64_t>( *u )
                   : fallback;
    }
    if( const double *d = v->real_if() ) {
        return std::isfinite( *d ) && std::abs( *d ) < 9.0e18 ? static_cast<std::int64_t>( *d )
                                                              : fallback;
    }
    return fallback;
}

double Preferences::get_number( std::string_view key, double fallback ) const noexcept
{
    const data::JsonValue *v = find( key );
    if( v == nullptr ) {
        return fallback;
    }
    if( const double *d = v->real_if() ) {
        return *d;
    }
    if( const std::int64_t *i = v->signed_integer_if() ) {
        return static_cast<double>( *i );
    }
    if( const std::uint64_t *u = v->unsigned_integer_if() ) {
        return static_cast<double>( *u );
    }
    return fallback;
}

std::string Preferences::get_text( std::string_view key, std::string_view fallback ) const
{
    const data::JsonValue *v = find( key );
    const std::string *s = v != nullptr ? v->string_if() : nullptr;
    return s != nullptr ? *s : std::string( fallback );
}

std::vector<std::string> Preferences::keys() const
{
    std::vector<std::string> out;
    if( const data::JsonValue::Object *o = settings_.object_if() ) {
        for( const auto &[key, value] : *o ) {
            out.push_back( key );
        }
    }
    return out;
}

PreferencesError Preferences::set( std::string_view key, data::JsonValue value )
{
    if( key.empty() ) {
        return { PreferencesErrorCode::invalid_key, "a setting's key is empty" };
    }
    data::JsonValue::Object *o = settings_.object_if();
    ( *o )[std::string( key )] = std::move( value );
    return save();
}

PreferencesError Preferences::reset( std::string_view key )
{
    data::JsonValue::Object *o = settings_.object_if();
    const auto at = o->find( key );
    if( at == o->end() ) {
        return {};
    }
    o->erase( at );
    return save();
}

PreferencesError Preferences::reset_all()
{
    settings_ = data::JsonValue( data::JsonValue::Object{} );
    return save();
}

PreferencesError Preferences::save()
{
    ++generation_;
    const data::JsonWriteResult text =
        data::write_json( settings_, { .pretty = true, .newline_at_end = true } );
    if( text.error ) {
        return { PreferencesErrorCode::invalid_settings, text.error.message };
    }
    const std::string at = folder_ + "/tree/" + std::string( settings_file );
    if( const files::FileError e =
            files_->write_atomically( FileType::Local, at, bytes_of( text.text ) ) ) {
        return io_error( e );
    }
    return {};
}

// ---- the app's files ------------------------------------------------------------------------------

std::string Preferences::path( std::string_view name ) const
{
    const std::string relative = tree_relative( name );
    return relative.empty() ? std::string() : folder_ + "/tree/" + relative;
}

files::FileReadResult Preferences::read( std::string_view name ) const
{
    const std::string at = path( name );
    if( at.empty() ) {
        files::FileReadResult r;
        r.error = { files::FileErrorCode::InvalidPath, "not a name in the preferences' tree",
                    std::string( name ) };
        return r;
    }
    return files_->read( FileType::Local, at );
}

PreferencesError Preferences::write( std::string_view name, std::span<const std::byte> bytes )
{
    const std::string at = path( name );
    if( at.empty() ) {
        return { PreferencesErrorCode::invalid_name,
                 std::string( name ) + ": not a name in the preferences' tree" };
    }
    if( const std::size_t slash = at.rfind( '/' ); slash != std::string::npos ) {
        if( const files::FileError e =
                files_->make_directories( FileType::Local, at.substr( 0, slash ) ) ) {
            return io_error( e );
        }
    }
    if( const files::FileError e = files_->write_atomically( FileType::Local, at, bytes ) ) {
        return io_error( e );
    }
    ++generation_;
    return {};
}

PreferencesError Preferences::remove( std::string_view name )
{
    const std::string at = path( name );
    if( at.empty() ) {
        return { PreferencesErrorCode::invalid_name,
                 std::string( name ) + ": not a name in the preferences' tree" };
    }
    if( const files::FileError e = files_->remove( FileType::Local, at ) ) {
        return io_error( e );
    }
    ++generation_;
    return {};
}

std::vector<std::string> Preferences::list( std::string_view folder ) const
{
    std::string at = folder_ + "/tree";
    if( !folder.empty() ) {
        const std::string relative = tree_relative( folder );
        if( relative.empty() ) {
            return {};
        }
        at += "/" + relative;
    }
    std::vector<std::string> names;
    if( files_->list( FileType::Local, at, names ) ) {
        return {};
    }
    std::ranges::sort( names );
    return names;
}

} // namespace sq::preferences
