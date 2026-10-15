#include <squared/dev/dev_log.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace
{

// Keeps an allocation observable, so the optimiser cannot elide a new/delete
// pair the test is counting.
void *volatile g_sink = nullptr;

// A listener on an OS-chosen loopback port, standing in for `nc -klu`.
struct Listener
{
        int fd = -1;
        std::uint16_t port = 0;

        Listener()
        {
            fd = ::socket( AF_INET, SOCK_DGRAM, 0 );
            sockaddr_in a{};
            a.sin_family = AF_INET;
            a.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
            a.sin_port = 0;
            const int bound = ::bind( fd, reinterpret_cast<sockaddr *>( &a ), sizeof( a ) );
            assert( bound == 0 );
            socklen_t len = sizeof( a );
            const int named = ::getsockname( fd, reinterpret_cast<sockaddr *>( &a ), &len );
            assert( named == 0 );
            static_cast<void>( bound );
            static_cast<void>( named );
            port = ntohs( a.sin_port );
            timeval timeout{ 0, 200000 };
            ::setsockopt( fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof( timeout ) );
        }
        ~Listener() { ::close( fd ); }
        Listener( const Listener & ) = delete;
        Listener &operator=( const Listener & ) = delete;

        /// The next line, or an empty string after the timeout.
        std::string receive() const
        {
            char buffer[1100];
            const ssize_t n = ::recv( fd, buffer, sizeof( buffer ), 0 );
            return n > 0 ? std::string( buffer, static_cast<std::size_t>( n ) ) : std::string();
        }
};

// Two allocation sites the reporting test can tell apart. Not inlined, so each
// is a frame of its own in the recorded stack.
[[gnu::noinline]] void allocate_here()
{
    auto value = std::make_unique<std::uint32_t>( 3 );
    g_sink = value.get();
}

[[gnu::noinline]] void allocate_there()
{
    auto value = std::make_unique<std::uint32_t>( 4 );
    g_sink = value.get();
}

bool starts_with( const std::string &text, std::string_view prefix )
{
    return text.compare( 0, prefix.size(), prefix ) == 0;
}

} // namespace

int main()
{
    using sq::dev::frame_begin;
    using sq::dev::frame_end;
    using sq::dev::FrameAllocations;

    // Not counting: frame_end() without frame_begin() reports nothing.
    assert( frame_end().count == 0 );

    // A frame that allocates nothing.
    frame_begin();
    int stack_value = 7;
    g_sink = &stack_value;
    FrameAllocations frame = frame_end();
    assert( frame.count == 0 && frame.bytes == 0 );

    // One allocation, from inline code.
    frame_begin();
    auto owned = std::make_unique<std::uint64_t>( 1 );
    g_sink = owned.get();
    frame = frame_end();
    assert( frame.count == 1 );
    assert( frame.bytes == sizeof( std::uint64_t ) );
    owned.reset();

    // An allocation made inside the shared C++ library (string growth is
    // compiled there, not inline) is counted: the replacement reaches it.
    std::string text;
    frame_begin();
    text.assign( 200, 'x' );
    frame = frame_end();
    assert( frame.count >= 1 && frame.bytes >= 200 );

    // Over-aligned new uses its own replacement and is counted.
    frame_begin();
    void *aligned = ::operator new( 64, std::align_val_t{ 64 } );
    frame = frame_end();
    assert( frame.count == 1 );
    assert( reinterpret_cast<std::uintptr_t>( aligned ) % 64 == 0 );
    ::operator delete( aligned, std::align_val_t{ 64 } );

    // Another thread allocating during the frame is not counted.
    std::atomic<int> step{ 0 };
    std::thread loader( [&step] {
        while( step.load() != 1 ) {
        }
        std::vector<int> work( 1000 );
        g_sink = work.data();
        step.store( 2 );
    } );
    frame_begin();
    step.store( 1 );
    while( step.load() != 2 ) {
    }
    frame = frame_end();
    loader.join();
    assert( frame.count == 0 );

    // Reporting: the first allocating frame at once, then a summary per 120
    // frames, and nothing for frames that allocate nothing.
    Listener listener;
    const auto status = sq::dev::open_log_sink( { .port = listener.port, .catch_crashes = false } );
    assert( status == sq::dev::LogSinkStatus::Opened );
    static_cast<void>( status );

    // The frames above already reported the first allocating frame and named
    // their sites (with no sink open, so those lines went nowhere). Finish the
    // current 120-frame window with clean frames and drain what it sent.
    for( int i = 0; i < 120; ++i ) {
        frame_begin();
        frame_end();
    }
    while( !listener.receive().empty() ) {
    }

    // Two windows. The first has five allocating frames from two new sites:
    // three from allocate_here(), two from allocate_there(). The second has
    // one, from the same allocate_here() call, which is a known site by then.
    // (A site includes its callers, so it has to be the same call expression.)
    for( int i = 0; i < 240; ++i ) {
        frame_begin();
        if( i < 3 || i == 120 ) {
            allocate_here();
        } else if( i < 5 ) {
            allocate_there();
        }
        frame_end();
    }
    std::vector<std::string> lines;
    for( std::string next = listener.receive(); !next.empty(); next = listener.receive() ) {
        std::printf( "  %s\n", next.c_str() );
        lines.push_back( next );
    }
    // Two "site" lines, each naming this program with an offset addr2line
    // takes; the summary and the by-site line; then the second window's
    // summary and by-site line, with no new site.
    assert( lines.size() == 6 );
    assert( starts_with( lines[0], "alloc: site " ) );
    assert( starts_with( lines[1], "alloc: site " ) );
    assert( lines[0].find( "frame_allocations_test 0x" ) != std::string::npos );
    assert( lines[1].find( "frame_allocations_test 0x" ) != std::string::npos );
    assert( lines[0] != lines[1] );
    assert( starts_with( lines[2],
                         "alloc: 5 of the last 120 frames allocated: 5 allocations, 20 bytes" ) );
    assert( starts_with( lines[3], "alloc: by site: " ) );
    assert( lines[3].find( ":3 " ) != std::string::npos );
    assert( lines[3].compare( lines[3].size() - 2, 2, ":2" ) == 0 );
    assert( starts_with( lines[4],
                         "alloc: 1 of the last 120 frames allocated: 1 allocations, 4 bytes" ) );
    // The by-site line names the same site as allocate_here()'s first line.
    const std::string here =
        lines[3].substr( std::string( "alloc: by site: " ).size(), lines[3].find( ':', 16 ) - 16 );
    assert( lines[5] == "alloc: by site: " + here + ":1" );

    // A clean window sends nothing.
    for( int i = 0; i < 120; ++i ) {
        frame_begin();
        frame_end();
    }
    assert( listener.receive().empty() );
    sq::dev::close_log_sink();

    std::printf( "frame allocations: all assertions passed\n" );
    return 0;
}
