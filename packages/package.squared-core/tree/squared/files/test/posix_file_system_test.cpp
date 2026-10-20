// PosixFileSystem against real directories under $TMPDIR: every operation,
// its failures, and how many allocations the everyday ones make.

#include <squared/files/posix_file_system.hpp>

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>
#include <vector>

namespace
{
std::size_t allocations = 0;
} // namespace

void *operator new( std::size_t size )
{
    ++allocations;
    void *p = std::malloc( size == 0 ? 1 : size ); // NOLINT(cppcoreguidelines-no-malloc)
    if( p == nullptr ) {
        std::abort();
    }
    return p;
}
void operator delete( void *p ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}
void operator delete( void *p, std::size_t /*size*/ ) noexcept
{
    std::free( p ); // NOLINT(cppcoreguidelines-no-malloc)
}

using namespace sq::files;

namespace
{

std::vector<std::byte> bytes_of( std::size_t count, unsigned char fill )
{
    return std::vector<std::byte>( count, static_cast<std::byte>( fill ) );
}

void remove_tree( const std::string &path )
{
    const std::string command = "rm -rf '" + path + "'";
    static_cast<void>( std::system( command.c_str() ) );
}

} // namespace

int main()
{
    const char *tmpdir = std::getenv( "TMPDIR" );
    const std::string base =
        std::string{ tmpdir != nullptr ? tmpdir : "/tmp" } + "/sq_posix_fs_test";
    remove_tree( base );
    assert( ::mkdir( base.c_str(), 0755 ) == 0 );
    assert( ::mkdir( ( base + "/internal" ).c_str(), 0755 ) == 0 );
    assert( ::mkdir( ( base + "/internal/SQ-INF" ).c_str(), 0755 ) == 0 );
    assert( ::mkdir( ( base + "/internal/skins" ).c_str(), 0755 ) == 0 );
    assert( ::mkdir( ( base + "/local" ).c_str(), 0755 ) == 0 );

    PosixFileSystem fs{ PosixFileSystemRoots{
        .internal_root = base + "/internal", .local_root = base + "/local", .external_root = {} } };
    const auto local = FileType::Local;
    const auto kilo = bytes_of( 1000, 0x5A );
    const auto tail = bytes_of( 24, 0xA5 );
    std::vector<std::string> names;

    // --- refusals, before anything touches the disk -------------------------
    assert( fs.write( FileType::Internal, "x", kilo, false ).code == FileErrorCode::ReadOnly );
    assert( fs.make_directories( FileType::Internal, "d" ).code == FileErrorCode::ReadOnly );
    assert( fs.remove( FileType::Internal, "skins" ).code == FileErrorCode::ReadOnly );
    assert( fs.write( local, "../x", kilo, false ).code == FileErrorCode::NotSupported );
    assert( fs.write( local, "a/../x", kilo, false ).code == FileErrorCode::NotSupported );
    assert( fs.write( local, "/x", kilo, false ).code == FileErrorCode::NotSupported );
    assert( fs.write( FileType::External, "x", kilo, false ).code == FileErrorCode::NotSupported );
    assert( !fs.exists( local, "../local" ) );
    assert( fs.resolve_platform_path( local, "a/.." ).empty() );
    assert( fs.resolve_platform_path( local, "..b" ) == base + "/local/..b" ); // a name, not ..
    assert( fs.resolve_platform_path( local, "" ) == base + "/local" );
    assert( fs.resolve_platform_path( local, "x/y" ) == base + "/local/x/y" );
    assert( fs.resolve_platform_path( FileType::Absolute, "/abs" ) == "/abs" );

    // --- directories, writing, reading ----------------------------------------
    assert( !fs.make_directories( local, "a/b/c" ) );
    assert( !fs.make_directories( local, "a/b/c" ) ); // already there: fine
    assert( fs.is_directory( local, "a/b/c" ) );
    assert( !fs.write( local, "a/b/c/f.bin", kilo, false ) );
    assert( !fs.write( local, "a/b/c/f.bin", tail, true ) );
    assert( fs.length( local, "a/b/c/f.bin" ) == 1024 );
    assert( fs.exists( local, "a/b/c/f.bin" ) && !fs.is_directory( local, "a/b/c/f.bin" ) );
    {
        const auto read = fs.read( local, "a/b/c/f.bin" );
        assert( read && read.bytes.size() == 1024 );
        assert( read.bytes.front() == kilo.front() && read.bytes.back() == tail.back() );
    }
    assert( !fs.write( local, "a/b/c/f.bin", tail, false ) ); // replaces
    assert( fs.length( local, "a/b/c/f.bin" ) == 24 );
    assert( !fs.write( local, "empty", {}, false ) );
    assert( fs.exists( local, "empty" ) && fs.length( local, "empty" ) == 0 );
    assert( fs.read( local, "empty" ) && fs.read( local, "empty" ).bytes.empty() );

    // --- failures ---------------------------------------------------------------
    assert( fs.read( local, "missing" ).error.code == FileErrorCode::NotFound );
    assert( fs.read( local, "missing" ).error.path == base + "/local/missing" );
    assert( fs.read( local, "a" ).error.code == FileErrorCode::IsADirectory );
    assert( fs.length( local, "a" ) == 0 && fs.length( local, "missing" ) == 0 );
    assert( !fs.exists( local, "missing" ) );
    assert( fs.list( local, "missing", names ).code == FileErrorCode::NotFound );
    assert( fs.list( local, "a/b/c/f.bin", names ).code == FileErrorCode::NotADirectory );
    assert( fs.make_directories( local, "a/b/c/f.bin" ).code == FileErrorCode::NotADirectory );
    assert( fs.make_directories( local, "a/b/c/f.bin/d" ).code == FileErrorCode::NotADirectory );
    assert( fs.write( local, "nodir/f", kilo, false ).code == FileErrorCode::NotFound );
    assert( fs.write( local, "a", kilo, false ).code == FileErrorCode::IsADirectory );
    assert( fs.remove( local, "a/b" ).code == FileErrorCode::IoFailure ); // not empty
    assert( fs.remove( local, "missing" ).code == FileErrorCode::NotFound );

    // --- listing ------------------------------------------------------------------
    assert( !fs.list( local, "a/b/c", names ) && names.size() == 1 && names[0] == "f.bin" );
    assert( !fs.list( local, "", names ) );
    std::sort( names.begin(), names.end() );
    assert( ( names == std::vector<std::string>{ "a", "empty" } ) );
    // SQ-INF is hidden at Internal's root, as on a device, and only there.
    assert( !fs.list( FileType::Internal, "", names ) );
    assert( names.size() == 1 && names[0] == "skins" );
    assert( fs.is_directory( FileType::Internal, "SQ-INF" ) );

    // --- removing -----------------------------------------------------------------
    assert( !fs.remove( local, "a/b/c/f.bin" ) && !fs.exists( local, "a/b/c/f.bin" ) );
    assert( !fs.remove( local, "a/b/c" ) && !fs.exists( local, "a/b/c" ) ); // empty directory

    // --- one allocation per call: the resolved path, and read's buffer ----------
    {
        const auto data = bytes_of( 4096, 0x42 );
        names.reserve( 8 );
        static_cast<void>( fs.make_directories( local, "bench" ) );
        const std::size_t before = allocations;
        assert( !fs.write( local, "bench/save.bin", data, false ) );
        assert( fs.exists( local, "bench/save.bin" ) );
        assert( fs.length( local, "bench/save.bin" ) == data.size() );
        assert( fs.read( local, "bench/save.bin" ).bytes.size() == data.size() );
        assert( !fs.list( local, "bench", names ) && names.size() == 1 );
        assert( !fs.remove( local, "bench/save.bin" ) );
        std::printf( "posix file system: 6 calls, %zu allocations\n", allocations - before );
        assert( allocations - before == 7 ); // six paths and one read buffer
    }

    remove_tree( base );
    std::printf( "posix file system: all assertions passed  sizeof(PosixFileSystem)=%zu\n",
                 sizeof( PosixFileSystem ) );
    return 0;
}
