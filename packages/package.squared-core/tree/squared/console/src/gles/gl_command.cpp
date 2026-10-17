// The Console's `gl` command: what the driver says about itself. Raw GL, as
// the renderer is; compiled with the gles backend only.

#include <squared/console/renderer.hpp>

#include <GLES3/gl3.h>

#include <array>

namespace sq::console
{

namespace
{

const char *text( GLenum name ) noexcept
{
    const GLubyte *value = glGetString( name );
    return value != nullptr ? reinterpret_cast<const char *>( value ) : "?";
}

GLint integer( GLenum name ) noexcept
{
    GLint value = 0;
    glGetIntegerv( name, &value );
    return value;
}

void gl( Console &console, const Args &, void * ) noexcept
{
    console.printf( Level::info, "gl: %s; %s; %s", text( GL_VERSION ), text( GL_RENDERER ),
                    text( GL_VENDOR ) );
    console.printf( Level::info, "glsl: %s", text( GL_SHADING_LANGUAGE_VERSION ) );
    std::array<GLint, 2> viewport{};
    glGetIntegerv( GL_MAX_VIEWPORT_DIMS, viewport.data() );
    console.printf( Level::info,
                    "limits: texture %d, renderbuffer %d, viewport %dx%d, vertex attributes %d, "
                    "texture units %d (%d in all)",
                    integer( GL_MAX_TEXTURE_SIZE ), integer( GL_MAX_RENDERBUFFER_SIZE ),
                    viewport[0], viewport[1], integer( GL_MAX_VERTEX_ATTRIBS ),
                    integer( GL_MAX_TEXTURE_IMAGE_UNITS ),
                    integer( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS ) );
    console.printf( Level::info, "extensions: %d", integer( GL_NUM_EXTENSIONS ) );
}

} // namespace

Commands::AddResult add_gl_command( Console &console ) noexcept
{
    return console.add( "gl", "gl: the GL driver, its version and its limits", gl );
}

} // namespace sq::console
