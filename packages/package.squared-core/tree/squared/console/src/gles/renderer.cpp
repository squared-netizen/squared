// The Console's GLES 3.0 renderer: layout() decides where every character
// goes; this file turns its quads into one indexed draw per 512 of them.
//
// Raw GL rather than the gles module's wrappers, like the sprite batch: the
// vertex is 8 bytes of integers (VertexArray takes floats only), the objects
// live on the default vertex array as the sprite batch's do, and a failure
// here is a bool rather than a GlesError, which owns a std::string.

#include <squared/console/layout.hpp>
#include <squared/console/renderer.hpp>
#include <squared/console/src/colour.hpp>
#include <squared/console/src/font8x8.hpp>

#include <GLES3/gl3.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace sq::console
{

namespace
{

constexpr const char *vertex_shader = R"(#version 300 es
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texel;
layout(location = 2) in float a_colour;

uniform vec2 u_screen;
uniform vec4 u_palette[7];

out vec2 v_texel;
out vec4 v_colour;

void main() {
    v_texel = a_texel / vec2(128.0, 64.0);
    v_colour = u_palette[int(a_colour)];
    gl_Position = vec4(a_position.x / u_screen.x * 2.0 - 1.0,
                       1.0 - a_position.y / u_screen.y * 2.0, 0.0, 1.0);
}
)";

constexpr const char *fragment_shader = R"(#version 300 es
precision mediump float;

in vec2 v_texel;
in vec4 v_colour;

uniform sampler2D u_atlas;

out vec4 fragment_colour;

void main() {
    fragment_colour = vec4(v_colour.rgb, v_colour.a * texture(u_atlas, v_texel).r);
}
)";

// Straight alpha, in Colour's order.
constexpr std::array<float, colour_count * 4> palette = {
    0.86F, 0.88F, 0.90F, 1.00F, // text
    1.00F, 0.78F, 0.30F, 1.00F, // warning
    1.00F, 0.42F, 0.40F, 1.00F, // error
    0.55F, 0.60F, 0.66F, 1.00F, // dim
    0.04F, 0.05F, 0.07F, 0.88F, // background
    0.30F, 0.55F, 0.95F, 0.85F, // cursor
    0.62F, 0.78F, 1.00F, 1.00F, // title
};

GLuint compile( GLenum stage, const char *source ) noexcept
{
    const GLuint shader = glCreateShader( stage );
    if( shader == 0 ) {
        return 0;
    }
    glShaderSource( shader, 1, &source, nullptr );
    glCompileShader( shader );
    GLint compiled = GL_FALSE;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        glDeleteShader( shader );
        return 0;
    }
    return shader;
}

GLuint link() noexcept
{
    const GLuint vertex = compile( GL_VERTEX_SHADER, vertex_shader );
    const GLuint fragment = vertex != 0 ? compile( GL_FRAGMENT_SHADER, fragment_shader ) : 0;
    const GLuint program = fragment != 0 ? glCreateProgram() : 0;
    if( program != 0 ) {
        glAttachShader( program, vertex );
        glAttachShader( program, fragment );
        glLinkProgram( program );
        glDetachShader( program, vertex );
        glDetachShader( program, fragment );
    }
    if( vertex != 0 ) {
        glDeleteShader( vertex );
    }
    if( fragment != 0 ) {
        glDeleteShader( fragment );
    }
    GLint linked = GL_FALSE;
    if( program != 0 ) {
        glGetProgramiv( program, GL_LINK_STATUS, &linked );
    }
    if( program != 0 && linked != GL_TRUE ) {
        glDeleteProgram( program );
        return 0;
    }
    return program;
}

std::int16_t pixel( int value ) noexcept
{
    return static_cast<std::int16_t>( std::clamp<int>( value,
                                                       std::numeric_limits<std::int16_t>::min(),
                                                       std::numeric_limits<std::int16_t>::max() ) );
}

} // namespace

Renderer::~Renderer()
{
    destroy();
}

bool Renderer::create() noexcept
{
    destroy();

    program_ = link();
    if( program_ == 0 ) {
        return false;
    }
    screen_uniform_ = glGetUniformLocation( program_, "u_screen" );
    palette_uniform_ = glGetUniformLocation( program_, "u_palette" );
    atlas_uniform_ = glGetUniformLocation( program_, "u_atlas" );

    // Both made here and gone when create() returns: 8 KB of texels and 6 KB of
    // indices, on the stack rather than kept.
    std::array<std::uint8_t, atlas_bytes> texels{};
    build_atlas( texels );
    glGenTextures( 1, &atlas_ );
    glBindTexture( GL_TEXTURE_2D, atlas_ );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, static_cast<GLsizei>( atlas_width ),
                  static_cast<GLsizei>( atlas_height ), 0, GL_RED, GL_UNSIGNED_BYTE,
                  texels.data() );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glBindTexture( GL_TEXTURE_2D, 0 );

    std::array<GLuint, 2> buffers{};
    glGenBuffers( 2, buffers.data() );
    vertex_buffer_ = buffers[0];
    index_buffer_ = buffers[1];

    // The batch, allocated once at full size and refilled; STREAM: the
    // driver may hand back fresh memory rather than wait for the GPU.
    glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer_ );
    glBufferData( GL_ARRAY_BUFFER, static_cast<GLsizeiptr>( sizeof( vertices_ ) ), nullptr,
                  GL_STREAM_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // Quad n is always the same six indices, so they are uploaded once.
    std::array<std::uint16_t, batch_quads * 6> indices{};
    for( std::size_t quad = 0; quad < batch_quads; ++quad ) {
        const auto base = static_cast<std::uint16_t>( quad * 4 );
        const std::size_t at = quad * 6;
        indices[at] = base;
        indices[at + 1] = static_cast<std::uint16_t>( base + 1 );
        indices[at + 2] = static_cast<std::uint16_t>( base + 2 );
        indices[at + 3] = base;
        indices[at + 4] = static_cast<std::uint16_t>( base + 2 );
        indices[at + 5] = static_cast<std::uint16_t>( base + 3 );
    }
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_buffer_ );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>( sizeof( indices ) ),
                  indices.data(), GL_STATIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    if( atlas_ == 0 || vertex_buffer_ == 0 || index_buffer_ == 0 || glGetError() != GL_NO_ERROR ) {
        destroy();
        return false;
    }
    return true;
}

void Renderer::draw( const Console &console, const Frame &frame, int width, int height ) noexcept
{
    if( !valid() || width <= 0 || height <= 0 ) {
        return;
    }
    glUseProgram( program_ );
    glUniform2f( screen_uniform_, static_cast<float>( width ), static_cast<float>( height ) );
    glUniform4fv( palette_uniform_, static_cast<GLsizei>( colour_count ), palette.data() );
    glUniform1i( atlas_uniform_, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, atlas_ );

    glDisable( GL_SCISSOR_TEST );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // The default vertex array, as the sprite batch uses: whoever draws next
    // specifies its own attributes.
    glBindVertexArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer_ );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_buffer_ );
    constexpr auto stride = static_cast<GLsizei>( sizeof( Vertex ) );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 2, GL_SHORT, GL_FALSE, stride, nullptr );
    // GL takes a buffer offset in a pointer parameter; there is no other spelling.
    // NOLINTBEGIN(performance-no-int-to-ptr)
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 2, GL_UNSIGNED_BYTE, GL_FALSE, stride,
                           reinterpret_cast<const void *>( offsetof( Vertex, u ) ) );
    glEnableVertexAttribArray( 2 );
    glVertexAttribPointer( 2, 1, GL_UNSIGNED_BYTE, GL_FALSE, stride,
                           reinterpret_cast<const void *>( offsetof( Vertex, colour ) ) );
    // NOLINTEND(performance-no-int-to-ptr)

    quads_ = 0;
    layout( console, frame, [this]( const Quad &quad ) { add( quad ); } );
    flush();

    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glUseProgram( 0 );
}

void Renderer::add( const Quad &quad ) noexcept
{
    if( quads_ == batch_quads ) {
        flush();
    }
    const auto u = static_cast<std::uint8_t>( ( quad.glyph % 16 ) * 8 );
    const auto v = static_cast<std::uint8_t>( ( quad.glyph / 16 ) * 8 );
    const auto colour = static_cast<std::uint8_t>( quad.colour );
    const std::int16_t left = pixel( quad.x );
    const std::int16_t top = pixel( quad.y );
    const std::int16_t right = pixel( quad.x + quad.width );
    const std::int16_t bottom = pixel( quad.y + quad.height );
    const auto u2 = static_cast<std::uint8_t>( u + 8 );
    const auto v2 = static_cast<std::uint8_t>( v + 8 );
    const std::size_t at = static_cast<std::size_t>( quads_ ) * 4;
    vertices_[at] = { left, top, u, v, colour, 0 };
    vertices_[at + 1] = { right, top, u2, v, colour, 0 };
    vertices_[at + 2] = { right, bottom, u2, v2, colour, 0 };
    vertices_[at + 3] = { left, bottom, u, v2, colour, 0 };
    ++quads_;
}

void Renderer::flush() noexcept
{
    if( quads_ == 0 ) {
        return;
    }
    glBufferSubData(
        GL_ARRAY_BUFFER, 0,
        static_cast<GLsizeiptr>( static_cast<std::size_t>( quads_ ) * 4 * sizeof( Vertex ) ),
        vertices_.data() );
    glDrawElements( GL_TRIANGLES, static_cast<GLsizei>( quads_ * 6 ), GL_UNSIGNED_SHORT, nullptr );
    quads_ = 0;
}

void Renderer::invalidate() noexcept
{
    // These names belong to the lost context; deleting them would free
    // unrelated objects in the new one.
    program_ = 0;
    vertex_buffer_ = 0;
    index_buffer_ = 0;
    atlas_ = 0;
    screen_uniform_ = -1;
    palette_uniform_ = -1;
    atlas_uniform_ = -1;
    quads_ = 0;
}

void Renderer::destroy() noexcept
{
    if( vertex_buffer_ != 0 || index_buffer_ != 0 ) {
        const std::array<GLuint, 2> buffers = { vertex_buffer_, index_buffer_ };
        glDeleteBuffers( 2, buffers.data() );
    }
    if( atlas_ != 0 ) {
        glDeleteTextures( 1, &atlas_ );
    }
    if( program_ != 0 ) {
        glDeleteProgram( program_ );
    }
    invalidate();
}

} // namespace sq::console
