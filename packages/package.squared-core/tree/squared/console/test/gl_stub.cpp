// A fake GLES 3.0 driver for the Console renderer's host test: exactly the
// entry points the renderer calls, defined against the real headers so the
// compiler checks every signature. It keeps names, bindings and the few bits
// of state the renderer promises to leave; it draws nothing.

#include "gl_stub.hpp"

#include <GLES3/gl3.h>

#include <algorithm>
#include <cstring>

namespace glstub
{

State state;

void reset() noexcept
{
    state = State{};
}

void reset_draws() noexcept
{
    state.draws = 0;
    state.indices_drawn = 0;
    state.largest_upload = 0;
    state.first_vertex_seen = false;
    state.calls = 0;
}

} // namespace glstub

using glstub::state;

namespace
{

GLuint make() noexcept
{
    ++state.created;
    return state.next_name++;
}

} // namespace

extern "C" {

GL_APICALL GLenum GL_APIENTRY glGetError( void )
{
    ++state.calls;
    return GL_NO_ERROR;
}
GL_APICALL GLuint GL_APIENTRY glCreateShader( GLenum )
{
    ++state.calls;
    return make();
}
GL_APICALL void GL_APIENTRY glShaderSource( GLuint, GLsizei, const GLchar *const *, const GLint * )
{
    ++state.calls;
}
GL_APICALL void GL_APIENTRY glCompileShader( GLuint )
{
    ++state.calls;
}
GL_APICALL void GL_APIENTRY glGetShaderiv( GLuint, GLenum, GLint *params )
{
    ++state.calls;
    *params = state.fail_compile ? GL_FALSE : GL_TRUE;
}
GL_APICALL void GL_APIENTRY glDeleteShader( GLuint )
{
    ++state.calls;
    ++state.deleted;
}
GL_APICALL GLuint GL_APIENTRY glCreateProgram( void )
{
    ++state.calls;
    return make();
}
GL_APICALL void GL_APIENTRY glAttachShader( GLuint, GLuint )
{
    ++state.calls;
}
GL_APICALL void GL_APIENTRY glDetachShader( GLuint, GLuint )
{
    ++state.calls;
}
GL_APICALL void GL_APIENTRY glLinkProgram( GLuint )
{
    ++state.calls;
}
GL_APICALL void GL_APIENTRY glGetProgramiv( GLuint, GLenum, GLint *params )
{
    ++state.calls;
    *params = GL_TRUE;
}
GL_APICALL void GL_APIENTRY glDeleteProgram( GLuint )
{
    ++state.calls;
    ++state.deleted;
}
GL_APICALL void GL_APIENTRY glUseProgram( GLuint program )
{
    ++state.calls;
    state.program = program;
}
GL_APICALL GLint GL_APIENTRY glGetUniformLocation( GLuint, const GLchar *name )
{
    ++state.calls;
    return std::strcmp( name, "u_screen" ) == 0 ? 0 : std::strcmp( name, "u_palette" ) == 0 ? 1 : 2;
}
GL_APICALL void GL_APIENTRY glUniform2f( GLint, GLfloat x, GLfloat y )
{
    ++state.calls;
    state.screen_width = x;
    state.screen_height = y;
}
GL_APICALL void GL_APIENTRY glUniform4fv( GLint, GLsizei count, const GLfloat * )
{
    ++state.calls;
    state.palette_entries = count;
}
GL_APICALL void GL_APIENTRY glUniform1i( GLint, GLint )
{
    ++state.calls;
}
GL_APICALL void GL_APIENTRY glGenTextures( GLsizei count, GLuint *names )
{
    ++state.calls;
    std::generate_n( names, count, make );
}
GL_APICALL void GL_APIENTRY glDeleteTextures( GLsizei count, const GLuint * )
{
    ++state.calls;
    state.deleted += count;
}
GL_APICALL void GL_APIENTRY glBindTexture( GLenum, GLuint texture )
{
    ++state.calls;
    state.texture = texture;
}
GL_APICALL void GL_APIENTRY glActiveTexture( GLenum )
{
    ++state.calls;
}
GL_APICALL void GL_APIENTRY glPixelStorei( GLenum name, GLint value )
{
    ++state.calls;
    if( name == GL_UNPACK_ALIGNMENT ) {
        state.unpack_alignment = value;
    }
}
GL_APICALL void GL_APIENTRY glTexImage2D( GLenum, GLint, GLint internal_format, GLsizei width,
                                          GLsizei height, GLint, GLenum, GLenum, const void * )
{
    ++state.calls;
    state.texture_internal_format = static_cast<unsigned>( internal_format );
    state.texture_width = width;
    state.texture_height = height;
}
GL_APICALL void GL_APIENTRY glTexParameteri( GLenum, GLenum, GLint )
{
    ++state.calls;
}
GL_APICALL void GL_APIENTRY glGenBuffers( GLsizei count, GLuint *names )
{
    ++state.calls;
    std::generate_n( names, count, make );
}
GL_APICALL void GL_APIENTRY glDeleteBuffers( GLsizei count, const GLuint * )
{
    ++state.calls;
    state.deleted += count;
}
GL_APICALL void GL_APIENTRY glBindBuffer( GLenum target, GLuint buffer )
{
    ++state.calls;
    ( target == GL_ARRAY_BUFFER ? state.array_buffer : state.element_buffer ) = buffer;
}
GL_APICALL void GL_APIENTRY glBufferData( GLenum target, GLsizeiptr size, const void *, GLenum )
{
    ++state.calls;
    const unsigned name = target == GL_ARRAY_BUFFER ? state.array_buffer : state.element_buffer;
    if( name < state.buffer_sizes.size() ) {
        state.buffer_sizes[name] = size;
    }
}
GL_APICALL void GL_APIENTRY glBufferSubData( GLenum, GLintptr, GLsizeiptr size, const void *data )
{
    ++state.calls;
    state.largest_upload = std::max( state.largest_upload, static_cast<std::ptrdiff_t>( size ) );
    if( !state.first_vertex_seen ) {
        std::memcpy( state.first_vertex.data(), data, state.first_vertex.size() );
        state.first_vertex_seen = true;
    }
}
GL_APICALL void GL_APIENTRY glBindVertexArray( GLuint )
{
    ++state.calls;
}
GL_APICALL void GL_APIENTRY glEnableVertexAttribArray( GLuint )
{
    ++state.calls;
}
GL_APICALL void GL_APIENTRY glVertexAttribPointer( GLuint, GLint, GLenum, GLboolean, GLsizei,
                                                   const void * )
{
    ++state.calls;
}
GL_APICALL void GL_APIENTRY glDrawElements( GLenum, GLsizei count, GLenum, const void * )
{
    ++state.calls;
    ++state.draws;
    state.indices_drawn += count;
}
GL_APICALL void GL_APIENTRY glEnable( GLenum capability )
{
    ++state.calls;
    if( capability == GL_SCISSOR_TEST ) {
        state.scissor = true;
    } else if( capability == GL_BLEND ) {
        state.blend = true;
    }
}
GL_APICALL void GL_APIENTRY glDisable( GLenum capability )
{
    ++state.calls;
    if( capability == GL_SCISSOR_TEST ) {
        state.scissor = false;
    } else if( capability == GL_BLEND ) {
        state.blend = false;
    }
}
GL_APICALL void GL_APIENTRY glBlendFunc( GLenum, GLenum )
{
    ++state.calls;
}
GL_APICALL const GLubyte *GL_APIENTRY glGetString( GLenum name )
{
    ++state.calls;
    ++state.strings_asked;
    const char *text = name == GL_VERSION    ? "OpenGL ES 3.0 stub"
                       : name == GL_RENDERER ? "stub renderer"
                       : name == GL_VENDOR   ? "squared"
                                             : nullptr; // the shading language: unknown
    return reinterpret_cast<const GLubyte *>( text );
}
GL_APICALL void GL_APIENTRY glGetIntegerv( GLenum name, GLint *data )
{
    ++state.calls;
    if( name == GL_MAX_VIEWPORT_DIMS ) {
        data[0] = 4096;
        data[1] = 2048;
        return;
    }
    data[0] = name == GL_MAX_TEXTURE_SIZE ? 4096 : name == GL_NUM_EXTENSIONS ? 42 : 16;
}

} // extern "C"
