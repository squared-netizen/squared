// The Console renderer against a fake driver (test/gl_stub.cpp): what it makes
// in create(), how it batches a frame, the state it leaves, context loss, and
// that a draw allocates nothing. It proves the call sequence; whether a real
// driver draws the right picture is the device check's job.

#include "gl_stub.hpp"

#include <squared/console/layout.hpp>
#include <squared/console/renderer.hpp>
#include <squared/dev/frame_allocations.hpp>

#include <GLES3/gl3.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

using namespace sq::console;

int main()
{
    auto console = std::make_unique<Console>();
    auto renderer = std::make_unique<Renderer>();

    // Nothing happens before create().
    glstub::reset();
    const Frame frame{ .x = 10, .y = 20, .width = 1060, .height = 2360, .cell = 24 };
    renderer->draw( *console, frame, 1080, 2400 );
    assert( glstub::state.calls == 0 && !renderer->valid() );

    // create(): a program, the font as one R8 texture, the two buffers.
    assert( renderer->create() && renderer->valid() );
    assert( glstub::state.texture_internal_format == GL_R8 );
    assert( glstub::state.texture_width == 128 && glstub::state.texture_height == 64 );
    assert( glstub::state.unpack_alignment == 4 ); // put back
    // Names: 1, 2 shaders; 3 program; 4 texture; 5, 6 buffers.
    assert( glstub::state.buffer_sizes[5] == 16384 && glstub::state.buffer_sizes[6] == 6144 );
    assert( glstub::state.created == 6 && glstub::state.deleted == 2 ); // the shaders
    assert( glstub::state.array_buffer == 0 && glstub::state.element_buffer == 0 &&
            glstub::state.texture == 0 );

    // A full screen: more than one batch, every quad drawn once.
    for( int i = 0; i < 400; ++i ) {
        console->printf( Level::info, "%04d the quick brown fox jumps over the lazy dog", i );
    }
    std::size_t quads = 0;
    layout( *console, frame, [&]( const Quad & ) { ++quads; } );
    assert( quads > 2 * Renderer::batch_quads );

    glstub::reset_draws();
    glstub::state.scissor = true; // as a clipped widget might leave it
    sq::dev::frame_begin();
    renderer->draw( *console, frame, 1080, 2400 );
    const sq::dev::FrameAllocations used = sq::dev::frame_end();
    assert( used.count == 0 );
    assert( static_cast<std::size_t>( glstub::state.indices_drawn ) == quads * 6 );
    assert( static_cast<std::size_t>( glstub::state.draws ) ==
            ( quads + Renderer::batch_quads - 1 ) / Renderer::batch_quads );
    assert( glstub::state.largest_upload == 16384 );
    assert( static_cast<std::size_t>( glstub::state.palette_entries ) == colour_count );
    assert( glstub::state.screen_width == 1080.0F && glstub::state.screen_height == 2400.0F );

    // The first quad is the background: the frame's corner, the solid glyph,
    // the background colour.
    std::int16_t x = 0;
    std::int16_t y = 0;
    std::memcpy( &x, &glstub::state.first_vertex[0], 2 );
    std::memcpy( &y, &glstub::state.first_vertex[2], 2 );
    assert( x == 10 && y == 20 );
    assert( glstub::state.first_vertex[4] == ( solid_glyph % 16 ) * 8 );
    assert( glstub::state.first_vertex[5] == ( solid_glyph / 16 ) * 8 );
    assert( glstub::state.first_vertex[6] == static_cast<std::uint8_t>( Colour::background ) );

    // What it leaves: blending on, no scissor, no program, no array buffer.
    assert( glstub::state.blend && !glstub::state.scissor );
    assert( glstub::state.program == 0 && glstub::state.array_buffer == 0 );

    // An empty console draws in one batch.
    console->run( "clear" );
    glstub::reset_draws();
    renderer->draw( *console, frame, 1080, 2400 );
    assert( glstub::state.draws == 1 );

    // A zero viewport draws nothing.
    glstub::reset_draws();
    renderer->draw( *console, frame, 0, 2400 );
    assert( glstub::state.calls == 0 );

    // Context loss: forget, delete nothing; then build again.
    const int deleted = glstub::state.deleted;
    renderer->invalidate();
    assert( !renderer->valid() && glstub::state.deleted == deleted );
    glstub::reset_draws();
    renderer->draw( *console, frame, 1080, 2400 );
    assert( glstub::state.calls == 0 );
    assert( renderer->create() && renderer->valid() );

    // destroy(): everything made is deleted.
    renderer->destroy();
    assert( !renderer->valid() );
    assert( glstub::state.created - 6 ==
            glstub::state.deleted - deleted ); // the second create's objects

    // A shader that does not compile: false, nothing left behind.
    glstub::reset();
    glstub::state.fail_compile = true;
    assert( !renderer->create() && !renderer->valid() );
    assert( glstub::state.created == glstub::state.deleted );

    renderer.reset(); // the destructor deletes nothing more
    assert( glstub::state.created == glstub::state.deleted );

    // The gl command: four lines, an unknown string as ?.
    assert( add_gl_command( *console ) == Commands::AddResult::added );
    console->run( "gl" );
    auto line = console->lines().last( 4 );
    assert( std::string( ( *line ).text ) == "gl: OpenGL ES 3.0 stub; stub renderer; squared" );
    ++line;
    assert( std::string( ( *line ).text ) == "glsl: ?" );
    ++line;
    assert( std::string( ( *line ).text ) ==
            "limits: texture 4096, renderbuffer 16, viewport 4096x2048, vertex attributes 16, "
            "texture units 16 (16 in all)" );
    ++line;
    assert( std::string( ( *line ).text ) == "extensions: 42" );
    assert( glstub::state.strings_asked == 4 );

    std::printf(
        "renderer: all assertions passed  sizeof(Renderer)=%zu  %zu quads in %d draws\n",
        sizeof( Renderer ), quads,
        static_cast<int>( ( quads + Renderer::batch_quads - 1 ) / Renderer::batch_quads ) );
    return 0;
}
