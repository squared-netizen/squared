#include <squared/gles/gles.hpp>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

namespace glstub {
void reset();
int count(const char* name);
bool deleted(const char* name, unsigned int object);
extern bool fail_compile;
extern bool fail_link;
extern int unpack_alignment;
}

using namespace sq::gles;

int main()
{
    glstub::reset();

    // --- Buffer -----------------------------------------------------------
    {
        Buffer buffer;
        const float data[] = {0.0F, 1.0F, 2.0F, 3.0F};
        assert(!buffer.create(BufferTarget::Vertex, BufferUsage::Static,
                              std::as_bytes(std::span{data})));
        assert(buffer.valid());
        assert(buffer.size_bytes() == sizeof(data));
        assert(buffer.target() == BufferTarget::Vertex);

        // an update past the end is refused before it reaches GL
        const int before = glstub::count("glBufferSubData");
        assert(buffer.update(sizeof(data), std::as_bytes(std::span{data})).code
               == GlesErrorCode::InvalidArgument);
        assert(glstub::count("glBufferSubData") == before);

        assert(!buffer.update(0, std::as_bytes(std::span{data})));

        // zero size is refused
        Buffer empty;
        assert(empty.create(BufferTarget::Vertex, BufferUsage::Static, {}, 0)
               .code == GlesErrorCode::InvalidArgument);
    }

    // --- the one that matters: invalidate() must not delete ---------------
    {
        glstub::reset();
        Buffer buffer;
        const float data[] = {1.0F};
        assert(!buffer.create(BufferTarget::Vertex, BufferUsage::Stream,
                              std::as_bytes(std::span{data})));
        const unsigned int name = buffer.name();
        buffer.invalidate();
        assert(!buffer.valid());
        assert(!glstub::deleted("glDeleteBuffers", name));
    }
    {
        glstub::reset();
        Texture2D texture;
        assert(!texture.create(4, 4, TextureFormat::Rgba8));
        const unsigned int name = texture.name();
        texture.invalidate();
        assert(!glstub::deleted("glDeleteTextures", name));
    }
    {
        glstub::reset();
        Program program;
        assert(!program.build("vs", "fs"));
        const unsigned int name = program.name();
        program.invalidate();
        assert(!glstub::deleted("glDeleteProgram", name));
    }

    // --- destroy() must delete, and only once ----------------------------
    {
        glstub::reset();
        Texture2D texture;
        assert(!texture.create(2, 2, TextureFormat::R8));
        const unsigned int name = texture.name();
        texture.destroy();
        texture.destroy();
        assert(glstub::deleted("glDeleteTextures", name));
        assert(glstub::count("glDeleteTextures") == 1);
    }

    // --- Shader / Program failure paths carry the driver log -------------
    {
        glstub::reset();
        glstub::fail_compile = true;
        Shader shader;
        const GlesError error = shader.compile(ShaderStage::Vertex, "bad");
        assert(error.code == GlesErrorCode::CompileFailed);
        assert(error.message.find("syntax error") != std::string::npos);
        assert(!shader.valid());
        glstub::fail_compile = false;

        glstub::fail_link = true;
        Program program;
        const GlesError link_error = program.build("vs", "fs");
        assert(link_error.code == GlesErrorCode::LinkFailed);
        assert(link_error.message.find("link error") != std::string::npos);
        glstub::fail_link = false;
    }

    // --- Program detaches its shaders after linking -----------------------
    {
        glstub::reset();
        Program program;
        assert(!program.build("vs", "fs"));
        assert(glstub::count("glAttachShader") == 2);
        assert(glstub::count("glDetachShader") == 2);
    }

    // --- introspection ----------------------------------------------------
    {
        glstub::reset();
        Program program;
        assert(!program.build("vs", "fs"));

        std::vector<AttributeInfo> attributes;
        assert(!program.attributes(attributes));
        assert(attributes.size() == 1);
        assert(attributes[0].name == "a_position");
        assert(attributes[0].location == 0);
        assert(std::string{glsl_type_name(attributes[0].type)} == "vec2");

        std::vector<UniformInfo> uniforms;
        assert(!program.uniforms(uniforms));
        assert(uniforms.size() == 1);
        assert(uniforms[0].name == "u_projection");
        assert(uniforms[0].location == 7);
        assert(std::string{glsl_type_name(uniforms[0].type)} == "mat4");

        // a uniform the linker removed reports -1, not an error
        assert(program.uniform_location("u_gone") == -1);

        // the vector is reused, not appended to
        assert(!program.uniforms(uniforms));
        assert(uniforms.size() == 1);
    }

    // --- Texture2D: alignment and sampling defaults ------------------------
    {
        glstub::reset();
        Texture2D texture;
        // width 3 in R8 is the classic misaligned case
        assert(!texture.create(3, 2, TextureFormat::R8));
        assert(glstub::unpack_alignment == 1);
        // create() sets sampling so a fresh texture is not black
        assert(glstub::count("glTexParameteri") == 4);

        // a region outside the texture is refused before it reaches GL
        const int before = glstub::count("glTexSubImage2D");
        const unsigned char pixels[] = {1, 2, 3, 4};
        assert(texture.upload_region(2, 0, 2, 2,
                   std::as_bytes(std::span{pixels})).code
               == GlesErrorCode::InvalidArgument);
        assert(glstub::count("glTexSubImage2D") == before);

        // a short pixel span is refused too
        Texture2D small;
        assert(small.create(4, 4, TextureFormat::Rgba8,
                   std::as_bytes(std::span{pixels})).code
               == GlesErrorCode::InvalidArgument);
    }

    // --- VertexArray rejects a buffer created for the wrong target --------
    {
        glstub::reset();
        VertexArray array;
        assert(!array.create());

        Buffer vertices;
        const float data[] = {0.0F, 1.0F};
        assert(!vertices.create(BufferTarget::Vertex, BufferUsage::Static,
                                std::as_bytes(std::span{data})));

        assert(array.set_indices(vertices).code
               == GlesErrorCode::InvalidArgument);

        const VertexAttribute layout[] = {
            {.location = 0, .components = 2, .stride = 8, .offset = 0},
            {.location = -1, .components = 2, .stride = 8, .offset = 0}
        };
        assert(!array.set_vertices(vertices, layout));
        // the -1 location is skipped, not passed to GL
        assert(glstub::count("glEnableVertexAttribArray") == 1);
    }

    // --- move semantics must not double-delete ----------------------------
    {
        glstub::reset();
        Texture2D first;
        assert(!first.create(2, 2, TextureFormat::Rgba8));
        const unsigned int name = first.name();
        {
            Texture2D second = std::move(first);
            assert(second.name() == name);
            assert(!first.valid());
        }
        assert(glstub::count("glDeleteTextures") == 1);
    }

    std::printf("gles: all assertions passed\n");
    std::printf("  sizeof(Buffer)=%zu Shader=%zu Program=%zu "
                "VertexArray=%zu Texture2D=%zu\n",
                sizeof(Buffer), sizeof(Shader), sizeof(Program),
                sizeof(VertexArray), sizeof(Texture2D));
    return 0;
}
