// Every public type in sq::gles, and the sq::graphics texture filter and wrap
// types Texture2D takes. Prefer a narrower include in headers of your own.

export module sq.gles;

export import :buffer;
export import :program;
export import :shader;
export import :src.attribute_info;
export import :src.buffer_target;
export import :src.buffer_usage;
export import :src.gles_error;
export import :src.gles_error_code;
export import :src.shader_stage;
export import :src.texture_format;
export import :src.uniform_info;
export import :src.vertex_attribute;
export import :texture2d;
export import :vertex_array;

export import sq.graphics;
