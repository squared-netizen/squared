#include <squared/files/posix_file_system.hpp>
#include <squared/files/src/file_type.hpp>
#include <squared/messaging/name_table.hpp>
#include <squared/messaging/telegram.hpp>
#include <squared/messaging/telegram_data.hpp>
#include <squared/messaging/telegram_text.hpp>
#include <squared/stage/save.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace sq
{

void Save::keep( const messaging::detail::Packed &packed, std::string_view name ) noexcept
{
    if( full_ ) {
        return;
    }
    // The key's own text, so the line reads as it was written: `set settings.volume 5.0`.
    std::array<std::byte, 256> table_storage{};
    messaging::NameTable names( table_storage );
    static_cast<void>( names.add( name ) );
    messaging::Telegram telegram = packed.telegram;
    if( !packed.data.empty() ) {
        // A text or uid list: where it is in the bytes that come with it.
        const messaging::DataRef where{ 0, static_cast<std::uint32_t>( packed.data.size() ) };
        telegram = messaging::Telegram::data( telegram.verb(), telegram.address(), telegram.type(),
                                              where );
    }
    const std::span<char> rest = out_.subspan( used_ );
    const std::size_t written = messaging::write_telegram( telegram, packed.data, names, rest );
    if( written == 0 || written >= rest.size() ) {
        full_ = true;
        return;
    }
    rest[written] = '\n';
    used_ += written + 1;
    ++lines_;
}

namespace detail
{

void give_back( std::span<const std::byte> saved,
                void ( *deliver )( void *context, const messaging::Received &received ) noexcept,
                void *context ) noexcept
{
    std::string_view text( reinterpret_cast<const char *>( saved.data() ), saved.size() );
    std::array<std::byte, 1024> storage{};
    while( !text.empty() ) {
        const std::size_t end = text.find( '\n' );
        const std::string_view line = text.substr( 0, end );
        text = end == std::string_view::npos ? std::string_view{} : text.substr( end + 1 );
        messaging::TelegramData data( storage );
        const std::optional<messaging::Telegram> read = messaging::read_telegram( line, data );
        // Only what a save writes: a line that is anything else is passed over.
        if( !read || read->verb() != messaging::Verb::set ) {
            continue;
        }
        messaging::Received received{ *read, {} };
        const messaging::ValueType type = read->type();
        if( type == messaging::ValueType::text || type == messaging::ValueType::uids ) {
            received.data = data.at( read->as_data() );
        }
        deliver( context, received );
    }
}

bool write_save( const ModelFile &file, std::string_view text ) noexcept
{
    // The platform's path, through a file system of the model's own: the
    // activity's may be gone by now. No roots: nothing but the absolute path.
    files::PosixFileSystem files( files::PosixFileSystemRoots{} );
    const auto *bytes = reinterpret_cast<const std::byte *>( text.data() );
    return !files.write_atomically( files::FileType::Absolute, file.path, { bytes, text.size() } );
}

} // namespace detail

} // namespace sq
