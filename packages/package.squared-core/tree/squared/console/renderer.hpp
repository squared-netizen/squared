#pragma once

#include <squared/console/console.hpp>
#include <squared/console/src/frame.hpp>
#include <squared/console/src/quad.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace sq::console
{

/**
 * @brief Draws a Console with GLES 3.0: one shader, one 8 KB font texture, and
 * one batch of 512 characters (16 KB) refilled as often as a frame needs.
 *
 * Built only with the gles graphics backend. Everything it needs is made in
 * create(), with the context current; draw() allocates nothing. It sets the GL
 * state it draws with (blending on; scissor, depth and culling off) and leaves
 * no program or array buffer bound; whatever draws next sets its own.
 *
 * ```cpp
 * sq::console::Renderer renderer;
 * if( !renderer.create() ) { ... }                    // once the context is current
 * renderer.draw( console, frame, pixel_width, pixel_height ); // after the app's frame
 * ```
 *
 * When the context is lost with its objects, call invalidate() and then
 * create() again; destroy() releases them while the context is current.
 */
class Renderer final
{
    public:
        /** @brief Characters (quads) in one batch; a frame with more draws more than once. */
        static constexpr std::size_t batch_quads = 512;

        Renderer() noexcept = default;
        ~Renderer();

        Renderer( const Renderer & ) = delete;
        Renderer &operator=( const Renderer & ) = delete;
        Renderer( Renderer && ) = delete;
        Renderer &operator=( Renderer && ) = delete;

        /**
         * @brief Compile the shader, upload the font, make the buffers. Needs a
         * current context. False if any of it failed; nothing is left behind.
         */
        [[nodiscard]] bool create() noexcept;

        /**
         * @brief Draw `console` into `frame`; the viewport is `width` by `height`
         * pixels. Does nothing until create() has succeeded.
         */
        void draw( const Console &console, const Frame &frame, int width, int height ) noexcept;

        /** @brief The context has gone with its objects: forget them without deleting. */
        void invalidate() noexcept;

        /** @brief Delete the GL objects. The context must be current. */
        void destroy() noexcept;

        [[nodiscard]] bool valid() const noexcept { return program_ != 0; }

    private:
        /// One corner of a character: position in pixels, font texel, palette index.
        struct Vertex final
        {
                std::int16_t x{ 0 };
                std::int16_t y{ 0 };
                std::uint8_t u{ 0 };
                std::uint8_t v{ 0 };
                std::uint8_t colour{ 0 };
                std::uint8_t pad{ 0 };
        };
        static_assert( sizeof( Vertex ) == 8 );

        void add( const Quad &quad ) noexcept;
        void flush() noexcept;

        std::array<Vertex, batch_quads * 4> vertices_{};
        std::uint32_t program_{ 0 };
        std::uint32_t vertex_buffer_{ 0 };
        std::uint32_t index_buffer_{ 0 };
        std::uint32_t atlas_{ 0 };
        std::int32_t screen_uniform_{ -1 };
        std::int32_t palette_uniform_{ -1 };
        std::int32_t atlas_uniform_{ -1 };
        std::uint32_t quads_{ 0 };
};

/**
 * @brief Add the `gl` command: the driver's vendor, renderer, version and
 * shading language, and the limits that decide what a program can draw
 * (texture and renderbuffer size, viewport, vertex attributes, texture
 * units), plus the extension count. Built with the gles backend, like the
 * Renderer; it must run where the context is current, as commands typed into
 * the Console do.
 */
[[nodiscard]] Commands::AddResult add_gl_command( Console &console ) noexcept;

} // namespace sq::console
