// The embedded backend: files compiled into the program, read through the
// FileSystem interface and a handle, as the Stage reads its default skin.

#include <squared/files/embedded_file_system.hpp>
#include <squared/files/file_handle.hpp>
#include <squared/files/src/file_error_code.hpp>
#include <squared/files/src/file_text_result.hpp>
#include <squared/files/src/file_type.hpp>

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{

#define CHECK( ... )                                                                               \
    do {                                                                                           \
        if( !( __VA_ARGS__ ) ) {                                                                   \
            std::fprintf( stderr, "embedded_file_system_test.cpp:%d: %s\n", __LINE__,              \
                          #__VA_ARGS__ );                                                          \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

using sq::files::EmbeddedFile;
using sq::files::EmbeddedFileSystem;
using sq::files::FileErrorCode;
using sq::files::FileType;

constexpr unsigned char hello[] = { 'h', 'e', 'l', 'l', 'o' };
constexpr unsigned char zeros[] = { 0, 1, 0 };

// A file directly at the root, two in a directory, one a level deeper, an empty one.
// clang-format off
constexpr std::array<EmbeddedFile, 5> table = { {
    { "top.txt", hello },
    { "skins/a.json", hello },
    { "skins/b.bin", zeros },
    { "skins/deep/c.txt", hello },
    { "skins/empty", {} },
} };
// clang-format on

void looks_up()
{
    const EmbeddedFileSystem files( table );
    CHECK( files.files().size() == 5 );
    CHECK( files.exists( FileType::Internal, "top.txt" ) );
    CHECK( files.exists( FileType::Internal, "skins" ) );
    CHECK( files.exists( FileType::Internal, "skins/deep" ) );
    CHECK( !files.exists( FileType::Internal, "skin" ) ); // a prefix, not a directory
    CHECK( !files.exists( FileType::Internal, "skins/" ) );
    CHECK( !files.exists( FileType::Local, "top.txt" ) ); // Internal only
    CHECK( files.is_directory( FileType::Internal, "skins" ) );
    CHECK( files.is_directory( FileType::Internal, "" ) ); // the root
    CHECK( !files.is_directory( FileType::Internal, "top.txt" ) );
    CHECK( files.length( FileType::Internal, "skins/b.bin" ) == 3 );
    CHECK( files.length( FileType::Internal, "skins" ) == 0 );
    CHECK( files.length( FileType::Internal, "nothing" ) == 0 );
    CHECK( files.exists( FileType::Internal, "skins/empty" ) );
}

void reads()
{
    const EmbeddedFileSystem files( table );
    const sq::files::FileReadResult bytes = files.read( FileType::Internal, "skins/b.bin" );
    CHECK( !bytes.error && bytes.bytes.size() == 3 );
    CHECK( bytes.bytes[0] == std::byte{ 0 } && bytes.bytes[1] == std::byte{ 1 } );
    CHECK( files.read( FileType::Internal, "skins/empty" ).bytes.empty() );
    CHECK( files.read( FileType::Internal, "nothing" ).error.code == FileErrorCode::NotFound );
    CHECK( files.read( FileType::Internal, "skins" ).error.code == FileErrorCode::IsADirectory );
    CHECK( files.read( FileType::Local, "top.txt" ).error.code == FileErrorCode::NotSupported );
}

void lists()
{
    const EmbeddedFileSystem files( table );
    std::vector<std::string> names;
    CHECK( !files.list( FileType::Internal, "skins", names ) );
    CHECK( names == std::vector<std::string>{ "a.json", "b.bin", "deep", "empty" } );
    CHECK( !files.list( FileType::Internal, "", names ) );
    CHECK( names == std::vector<std::string>{ "skins", "top.txt" } ); // each once, sorted
    const FileErrorCode file = files.list( FileType::Internal, "top.txt", names ).code;
    CHECK( file == FileErrorCode::NotADirectory );
    CHECK( files.list( FileType::Internal, "none", names ).code == FileErrorCode::NotFound );
    CHECK( names.empty() );
}

void refuses_writing()
{
    EmbeddedFileSystem files( table );
    const std::array<std::byte, 1> one{};
    CHECK( files.write( FileType::Internal, "x", one, false ).code == FileErrorCode::ReadOnly );
    CHECK( files.make_directories( FileType::Internal, "d" ).code == FileErrorCode::ReadOnly );
    CHECK( files.remove( FileType::Internal, "top.txt" ).code == FileErrorCode::ReadOnly );
    CHECK( files.exists( FileType::Internal, "top.txt" ) );
}

void through_a_handle()
{
    EmbeddedFileSystem files( table );
    const sq::files::FileTextResult text = files.internal( "skins/a.json" ).read_string();
    CHECK( text && text.text == "hello" );
    CHECK( files.internal( "skins/deep" ).exists() );
    CHECK( !files.local( "top.txt" ).exists() );
}

} // namespace

int main()
{
    looks_up();
    reads();
    lists();
    refuses_writing();
    through_a_handle();
    std::printf( "embedded_file_system_test: ok\n" );
    return 0;
}
