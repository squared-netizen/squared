#include <squared/graphics2d/sprite_batch.hpp>

#include <SDL.h>
#include <SDL_opengles2_khrplatform.h>
#include <SDL_opengles2_gl2platform.h>
#include <SDL_opengles2_gl2.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace squared::graphics2d {
namespace {

constexpr std::size_t kFloatsPerVertex = 8;
constexpr std::size_t kVerticesPerSprite = 4;
constexpr std::size_t kIndicesPerSprite = 6;
constexpr std::size_t kMaximumIndexableSprites = 16383;
constexpr float kPi = 3.14159265358979323846F;

constexpr const char* kVertexShader = R"(
attribute vec2 a_position;
attribute vec4 a_color;
attribute vec2 a_tex_coord;

uniform mat4 u_projection;

varying vec4 v_color;
varying vec2 v_tex_coord;

void main()
{
    v_color = a_color;
    v_tex_coord = a_tex_coord;
    gl_Position = u_projection * vec4(a_position, 0.0, 1.0);
}
)";

constexpr const char* kFragmentShader = R"(
precision mediump float;

varying vec4 v_color;
varying vec2 v_tex_coord;

uniform sampler2D u_texture;

void main()
{
    gl_FragColor = texture2D(u_texture, v_tex_coord) * v_color;
}
)";

GLuint compile_shader(GLenum type, const char* source) noexcept
{
    const GLuint shader = glCreateShader(type);
    if (!shader) return 0;
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) return shader;

    std::array<char, 1024> message{};
    glGetShaderInfoLog(
        shader,
        static_cast<GLsizei>(message.size()),
        nullptr,
        message.data()
    );
    SDL_Log("Squared sprite shader compilation failed: %s", message.data());
    glDeleteShader(shader);
    return 0;
}

GLuint create_program() noexcept
{
    const GLuint vertex = compile_shader(GL_VERTEX_SHADER, kVertexShader);
    if (!vertex) return 0;
    const GLuint fragment = compile_shader(
        GL_FRAGMENT_SHADER,
        kFragmentShader
    );
    if (!fragment) {
        glDeleteShader(vertex);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glBindAttribLocation(program, 0, "a_position");
    glBindAttribLocation(program, 1, "a_color");
    glBindAttribLocation(program, 2, "a_tex_coord");
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) return program;

    std::array<char, 1024> message{};
    glGetProgramInfoLog(
        program,
        static_cast<GLsizei>(message.size()),
        nullptr,
        message.data()
    );
    SDL_Log("Squared sprite shader link failed: %s", message.data());
    glDeleteProgram(program);
    return 0;
}

}  // namespace

SpriteBatch::~SpriteBatch()
{
    destroy();
}

bool SpriteBatch::initialize(std::size_t maximum_sprites) noexcept
{
    destroy();
    while (glGetError() != GL_NO_ERROR) {
    }
    maximum_sprites_ = std::min(
        std::max<std::size_t>(maximum_sprites, 1),
        kMaximumIndexableSprites
    );
    vertices_.reserve(
        maximum_sprites_ * kVerticesPerSprite * kFloatsPerVertex
    );

    program_ = create_program();
    if (!program_) {
        destroy();
        return false;
    }
    projection_uniform_ = glGetUniformLocation(program_, "u_projection");
    texture_uniform_ = glGetUniformLocation(program_, "u_texture");
    if (projection_uniform_ < 0 || texture_uniform_ < 0) {
        SDL_Log("Squared sprite shader uniforms are unavailable");
        destroy();
        return false;
    }

    std::vector<std::uint16_t> indices(
        maximum_sprites_ * kIndicesPerSprite
    );
    for (std::size_t sprite = 0; sprite < maximum_sprites_; ++sprite) {
        const auto vertex = static_cast<std::uint16_t>(
            sprite * kVerticesPerSprite
        );
        const std::size_t offset = sprite * kIndicesPerSprite;
        indices[offset] = vertex;
        indices[offset + 1] = vertex + 1;
        indices[offset + 2] = vertex + 2;
        indices[offset + 3] = vertex + 2;
        indices[offset + 4] = vertex + 3;
        indices[offset + 5] = vertex;
    }

    glGenBuffers(1, &vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            maximum_sprites_ *
            kVerticesPerSprite *
            kFloatsPerVertex *
            sizeof(float)
        ),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glGenBuffers(1, &index_buffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            indices.size() * sizeof(std::uint16_t)
        ),
        indices.data(),
        GL_STATIC_DRAW
    );

    if (!vertex_buffer_ || !index_buffer_ ||
        glGetError() != GL_NO_ERROR) {
        SDL_Log("Squared sprite batch buffer allocation failed");
        destroy();
        return false;
    }
    return true;
}

void SpriteBatch::destroy() noexcept
{
    drawing_ = false;
    vertices_.clear();
    sprite_count_ = 0;
    active_texture_ = 0;
    if (index_buffer_) glDeleteBuffers(1, &index_buffer_);
    if (vertex_buffer_) glDeleteBuffers(1, &vertex_buffer_);
    if (program_) glDeleteProgram(program_);
    index_buffer_ = 0;
    vertex_buffer_ = 0;
    program_ = 0;
    projection_uniform_ = -1;
    texture_uniform_ = -1;
    maximum_sprites_ = 0;
}

bool SpriteBatch::begin(const OrthographicCamera& camera) noexcept
{
    if (!valid() || drawing_) return false;
    drawing_ = true;
    active_texture_ = 0;
    sprite_count_ = 0;
    vertices_.clear();

    glUseProgram(program_);
    glUniformMatrix4fv(
        projection_uniform_,
        1,
        GL_FALSE,
        camera.combined().data()
    );
    glUniform1i(texture_uniform_, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    return true;
}

void SpriteBatch::draw(
    const TextureRegion& region,
    float x,
    float y,
    float width,
    float height,
    squared::graphics::Color color
) noexcept
{
    const float positions[]{
        x, y,
        x + width, y,
        x + width, y + height,
        x, y + height
    };
    append_quad(region, positions, color);
}

void SpriteBatch::draw(const Sprite& sprite) noexcept
{
    const float world_origin_x = sprite.x() + sprite.origin_x();
    const float world_origin_y = sprite.y() + sprite.origin_y();
    const float local_left = -sprite.origin_x() * sprite.scale_x();
    const float local_top = -sprite.origin_y() * sprite.scale_y();
    const float local_right =
        (sprite.width() - sprite.origin_x()) * sprite.scale_x();
    const float local_bottom =
        (sprite.height() - sprite.origin_y()) * sprite.scale_y();
    const float radians = sprite.rotation() * kPi / 180.0F;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    const auto transformed = [
        world_origin_x,
        world_origin_y,
        cosine,
        sine
    ](float x, float y) {
        return std::array<float, 2>{
            world_origin_x + x * cosine - y * sine,
            world_origin_y + x * sine + y * cosine
        };
    };

    const auto top_left = transformed(local_left, local_top);
    const auto top_right = transformed(local_right, local_top);
    const auto bottom_right = transformed(local_right, local_bottom);
    const auto bottom_left = transformed(local_left, local_bottom);
    const float positions[]{
        top_left[0], top_left[1],
        top_right[0], top_right[1],
        bottom_right[0], bottom_right[1],
        bottom_left[0], bottom_left[1]
    };
    append_quad(sprite.region(), positions, sprite.color());
}

void SpriteBatch::end() noexcept
{
    if (!drawing_) return;
    flush();
    drawing_ = false;
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

void SpriteBatch::flush() noexcept
{
    if (!drawing_ || sprite_count_ == 0 || active_texture_ == 0) return;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, active_texture_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        static_cast<GLsizeiptr>(vertices_.size() * sizeof(float)),
        vertices_.data()
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
    constexpr GLsizei stride =
        static_cast<GLsizei>(kFloatsPerVertex * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        stride,
        nullptr
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(2 * sizeof(float))
    );
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(6 * sizeof(float))
    );
    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(sprite_count_ * kIndicesPerSprite),
        GL_UNSIGNED_SHORT,
        nullptr
    );
    vertices_.clear();
    sprite_count_ = 0;
}

bool SpriteBatch::valid() const noexcept
{
    return program_ && vertex_buffer_ && index_buffer_;
}

void SpriteBatch::append_quad(
    const TextureRegion& region,
    const float* positions,
    squared::graphics::Color color
) noexcept
{
    if (!drawing_ || !region.valid()) return;
    const unsigned int texture = region.texture().native_handle();
    if (sprite_count_ > 0 &&
        (active_texture_ != texture ||
         sprite_count_ >= maximum_sprites_)) {
        flush();
    }
    active_texture_ = texture;

    const auto safe = color.clamped();
    const float normal_texture_coordinates[]{
        region.u1(), region.v1(),
        region.u2(), region.v1(),
        region.u2(), region.v2(),
        region.u1(), region.v2()
    };
    const float rotated_texture_coordinates[]{
        region.u2(), region.v1(),
        region.u2(), region.v2(),
        region.u1(), region.v2(),
        region.u1(), region.v1()
    };
    const float* texture_coordinates = region.rotated_clockwise()
        ? rotated_texture_coordinates
        : normal_texture_coordinates;
    for (std::size_t vertex = 0; vertex < kVerticesPerSprite; ++vertex) {
        vertices_.push_back(positions[vertex * 2]);
        vertices_.push_back(positions[vertex * 2 + 1]);
        vertices_.push_back(safe.red);
        vertices_.push_back(safe.green);
        vertices_.push_back(safe.blue);
        vertices_.push_back(safe.alpha);
        vertices_.push_back(texture_coordinates[vertex * 2]);
        vertices_.push_back(texture_coordinates[vertex * 2 + 1]);
    }
    ++sprite_count_;
}

}  // namespace squared::graphics2d
