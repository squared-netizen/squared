#pragma once

// What the fake driver in gl_stub.cpp saw. Counters and fixed arrays only, so
// recording a call allocates nothing and the renderer's own allocations can be
// counted around a draw.

#include <array>
#include <cstddef>
#include <cstdint>

namespace glstub
{

struct State
{
        unsigned next_name = 1;
        int created = 0; // shaders, programs, buffers and textures made
        int deleted = 0; // and deleted
        int calls = 0;   // every call
        bool fail_compile = false;
        bool scissor = true; // as an application might leave it
        bool blend = false;
        unsigned program = 0;
        unsigned array_buffer = 0;
        unsigned element_buffer = 0;
        unsigned texture = 0;
        int unpack_alignment = 4;
        int texture_width = 0;
        int texture_height = 0;
        unsigned texture_internal_format = 0;
        std::array<std::ptrdiff_t, 8> buffer_sizes{}; // by name, from glBufferData
        int draws = 0;
        int indices_drawn = 0;
        std::ptrdiff_t largest_upload = 0;
        std::array<std::uint8_t, 8> first_vertex{}; // of the first upload since reset_draws()
        bool first_vertex_seen = false;
        int palette_entries = 0;
        float screen_width = 0;
        float screen_height = 0;
        int strings_asked = 0;
};

extern State state;

void reset() noexcept;
void reset_draws() noexcept;

} // namespace glstub
